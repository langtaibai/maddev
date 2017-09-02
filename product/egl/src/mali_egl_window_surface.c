/* Copyright:
 * ----------------------------------------------------------------------------
 * This confidential and proprietary software may be used only as authorized
 * by a licensing agreement from ARM Limited.
 *      (C) COPYRIGHT 2011-2017 ARM Limited, ALL RIGHTS RESERVED
 * The entire notice above must be reproduced on all authorized copies and
 * copies may only be made to the extent permitted by a licensing agreement
 * from ARM Limited.
 * ----------------------------------------------------------------------------
 */

/** @file
  * Public API entry points and internal functions for EGL relating to surface management
  */

#include <cmem/mali_cmem.h>
#include <cdbg/mali_cdbg.h>
#include <osu/mali_osu.h>
#include <cinstr/mali_cinstr_helpers.h>
#include <cinstr/mali_cinstr_tracepoints.h>
#include <cutils/cstr/mali_cutils_cstr.h>
#include <cutils/misc/mali_cutils_misc.h>
#include <cobj/mali_cobj.h>
#include <cdbg/mali_cdbg_trace.h>
#include <cctx/mali_cctx_structs.h>

#include "mali_egl_khronos_header.h" /* Which includes EGL/egl.h */
#include "mali_egl_internal_types.h"
#include "mali_egl_surface_common.h"
#include "mali_egl_window_surface.h"
#include "mali_egl_display.h"
#include "mali_egl_config.h"
#include "mali_egl_context.h"
#include "mali_egl_thread_state.h"
#include "mali_egl_color_buffer_internal.h"
#include "mali_egl_misc.h"
#include "mali_egl_sync.h"

#define EGLP_DEFAULT_SWAP_INTERVAL 1

static const EGLint eglp_window_surface_default_attribs[] = { EGL_RENDER_BUFFER, EGL_BACK_BUFFER, EGL_GL_COLORSPACE_KHR,
	                                                          EGL_GL_COLORSPACE_LINEAR_KHR,
#if EGL_USE_PROTECTED_SURFACE
	                                                          EGL_PROTECTED_CONTENT_EXT, EGL_FALSE,
#endif
	                                                          EGL_NONE };

static egl_color_buffer_format get_intermediate_format(egl_color_buffer_format format)
{
	EGLint comp_sizes[4];
	EGLint res;
	EGLBoolean is_afbc;
	egl_color_buffer_format intermediate_format = EGL_COLOR_BUFFER_FORMAT_INVALID;
	cobj_surface_format surface_format = (cobj_surface_format)format;

	is_afbc = (GPU_TEXEL_ORDERING_AFBC == cobj_surface_format_get_texel_ordering(&surface_format));

	if (cobj_surface_format_is_yuv(&surface_format))
	{
		res = egl_color_buffer_get_yuva_comp_sizes(format, comp_sizes);
	}
	else
	{
		if (is_afbc)
		{
			/* For compressed format, we get pixel info by forcing linear ordering on the format*/
			cobj_surface_format_set_texel_ordering(&surface_format, GPU_TEXEL_ORDERING_LINEAR);
		}

		cobj_surface_format_pixel_info pixel_info;
		cobj_surface_format_get_pixel_info(&surface_format, &pixel_info);
		comp_sizes[0] = pixel_info.channels[0].nbits;
		comp_sizes[1] = pixel_info.channels[1].nbits;
		comp_sizes[2] = pixel_info.channels[2].nbits;
		comp_sizes[3] = pixel_info.channels[3].nbits;
		res = MALI_TRUE;
	}

	if (MALI_TRUE == res)
	{
		if (comp_sizes[0] <= 8 && comp_sizes[1] <= 8 && comp_sizes[2] <= 8 && comp_sizes[3] <= 8)
		{
			if (is_afbc)
			{
				intermediate_format = EGL_COLOR_BUFFER_FORMAT_BGR888_AFBC;
			}
			else
			{
				intermediate_format = EGL_COLOR_BUFFER_FORMAT_RGB888;
			}
		}
		if (comp_sizes[0] == 10 && comp_sizes[1] == 10 && comp_sizes[2] == 10 && comp_sizes[3] <= 2)
		{
			if (is_afbc)
			{
#if MALI_USE_BIFROST
				/* Bifrost requires the intermediate surface to have a tilebuffer format that
				 * matches the one use for the 10-bit YUV surface. The tilebuffer format used
				 * for 10-bit YUV is RGB8A2, and the surface format is therefore set to 241
				 * (R8G8B8A2_AU). */
				intermediate_format = EGL_COLOR_BUFFER_FORMAT_ABGR2888_AFBC;
#else
				intermediate_format = EGL_COLOR_BUFFER_FORMAT_ABGR2101010_AFBC;
#endif
			}
			else
			{
#if MALI_USE_BIFROST
				intermediate_format = EGL_COLOR_BUFFER_FORMAT_ABGR2888;
#else
				intermediate_format = EGL_COLOR_BUFFER_FORMAT_ABGR2101010;
#endif
			}
		}
	}

	return intermediate_format;
}

static mali_bool eglp_window_set_render_target(eglp_surface *surf)
{
	CDBG_ASSERT_POINTER(surf);

	mali_bool retval = MALI_TRUE;
	cobj_surface_template *surf_templ = NULL;
	cobj_surface_template *ms_templ = NULL;
	cdeps_tracker *dependency_tracker = NULL;
	mali_error err_res = MALI_ERROR_NONE;

	/* It's the caller's responsibility to handle cleanly a NULL current_color_buffer. */
	CDBG_ASSERT_POINTER(surf->current_color_buffer);

	/* If the render target is YUV, set an RGB888 intermediate buffer as a render target.
	 * The actual YUV render target will be set back during eglSwapBuffers. */
	if (cobj_surface_format_is_yuv((cobj_surface_format *)&surf->current_color_buffer->format))
	{
		egl_color_buffer_rotation rotation = egl_color_buffer_get_rotation(surf->current_color_buffer);
		EGLBoolean inverted = egl_color_buffer_get_y_inversion(surf->current_color_buffer);
		EGLBoolean secure = egl_color_buffer_is_secure(surf->current_color_buffer);

		/* If dimensions changed, release the current intermediate buffer so it gets reallocated */
		if (NULL != surf->intermediate_color_buffer &&
		    (surf->current_color_buffer->width != surf->intermediate_color_buffer->width ||
		     surf->current_color_buffer->height != surf->intermediate_color_buffer->height))
		{
			egl_color_buffer_release(surf->intermediate_color_buffer);
			surf->intermediate_color_buffer = NULL;
		}

		/* Allocate a new intermediate buffer with 'defer_alloc' set to true.
		 * It will effectively be allocated only if needed.*/
		if (NULL == surf->intermediate_color_buffer)
		{
			egl_color_buffer_format intermediate_format = get_intermediate_format(surf->current_color_buffer->format);

			surf->intermediate_color_buffer = egl_color_buffer_new(
			    surf->dpy, intermediate_format, surf->current_color_buffer->width, surf->current_color_buffer->height,
			    surf->config, inverted, secure, MALI_TRUE, NULL);

			if (NULL == surf->intermediate_color_buffer)
			{
				retval = MALI_FALSE;
				goto finish;
			}
		}

		/* Pass the rotation to the intermediate buffer */
		if (EGL_FALSE == egl_color_buffer_set_rotation(surf->intermediate_color_buffer, rotation))
		{
			retval = MALI_FALSE;
			goto finish;
		}

		surf->current_color_buffer = surf->intermediate_color_buffer;
	}

	/* Update sRGB information on the color buffer format */
	err_res = eglp_color_buffer_set_srgb(surf->current_color_buffer, surf->is_srgb);
	if (mali_error_is_error(err_res))
	{
		retval = MALI_FALSE;
		goto finish;
	}

	surf_templ = eglp_color_buffer_get_surface_template(surf->current_color_buffer);
	CDBG_ASSERT_POINTER(surf_templ);

	/* Multisample template setup */
	if (surf->config->secondary_attribs.sample_buffers != 0)
	{
		s32 num_samples = surf->config->primary_attribs.samples;
		cmem_properties ms_properties = BASE_MEM_PROT_GPU_RD | BASE_MEM_PROT_GPU_WR | BASE_MEM_PROT_CPU_WR;
		if (egl_color_buffer_is_secure(surf->current_color_buffer))
		{
			ms_properties |= BASE_MEM_SECURE;
		}
		else
		{
			ms_properties |= BASE_MEM_PROT_CPU_RD | BASE_MEM_GROW_ON_GPF;
		}

		u32 ms_width = cobj_surface_template_get_width(surf_templ);
		u32 ms_height = cobj_surface_template_get_height(surf_templ);
		cobj_surface_format ms_format = cobj_surface_template_get_format(surf_templ);
		ms_templ = cobj_surface_template_new(surf->dpy->common_ctx, ms_properties, ms_width, ms_height, num_samples,
		                                     ms_format, COBJ_SURFACE_USE_IMAGE | COBJ_SURFACE_USE_ATTRIBUTE |
		                                     COBJ_SURFACE_USE_FRAME_BUFFER_WRITEBACK |
		                                     COBJ_SURFACE_USE_FRAME_BUFFER_PRELOAD);
		if (ms_templ == NULL)
		{
			retval = MALI_FALSE;
			cobj_template_release(&surf_templ->super); /* exiting on error so drop the template reference */
			goto finish;
		}
	}

	dependency_tracker = eglp_color_buffer_get_dependency_tracker(surf->current_color_buffer);
	CDBG_ASSERT_POINTER(dependency_tracker);

	{
		/* If the surface changes size, we need to attach a new depth/stencil buffer */
		u32 curr_width = 0;
		u32 curr_height = 0;
#if MALI_USE_GLES
		gles_context_surface_get_dimensions(surf->gles_ctx_surface, &curr_width, &curr_height);
#endif
		u32 new_width = cobj_surface_template_get_width(surf_templ);
		u32 new_height = cobj_surface_template_get_height(surf_templ);

		if ((curr_width != new_width) || (curr_height != new_height))
		{
			retval = eglp_surface_set_depth_stencil(surf, new_width, new_height);
			if (!retval)
			{
				cobj_template_release(&surf_templ->super); /* exiting on error so drop the template reference */
				goto finish;
			}
		}
	}

#if MALI_USE_GLES
	err_res = gles_context_surface_set_render_target(surf->gles_ctx_surface, CPOM_RT_COLOR, surf_templ,
	                                                 dependency_tracker, MALI_FALSE, MALI_FALSE, ms_templ);

	/* GLES will have taken a reference to the cobj_surface_template, so we can drop our reference: */
	cobj_template_release(&surf_templ->super);

	CDBG_ASSERT((MALI_ERROR_OUT_OF_GPU_MEMORY == err_res) || (MALI_ERROR_OUT_OF_MEMORY == err_res) ||
	            (!mali_error_is_error(err_res)));
	if (mali_error_is_error(err_res))
	{
		retval = MALI_FALSE;
		goto finish;
	}

	mali_bool linear_order = (surf->render_buffer == EGL_SINGLE_BUFFER);
	gles_context_surface_set_tile_render_order(surf->gles_ctx_surface, linear_order);
#endif /* MALI_USE_GLES */

finish:
	if (ms_templ != NULL)
	{
		cobj_template_release(&ms_templ->super);
	}

	return retval;
}

static mali_bool eglp_handle_buffer_sync_mode(eglp_surface *surf)
{
	mali_bool retval = MALI_TRUE;
	eglp_thread_state *thread_data = eglp_get_current_thread_state();
	eglp_display *dpy = surf->dpy;
	egl_buffer_sync_method sync_method;

	if (NULL == thread_data)
	{
		retval = MALI_FALSE;
		goto finish;
	}

	if (NULL == surf->writeback_color_buffer)
	{
		retval = MALI_FALSE;
		goto finish;
	}

#if MALI_USE_GLES
	CDBG_ASSERT(NULL != thread_data->gles_ctx);

	/* If the sync method is timeline, and a platform fence is set, we have to make any rendering commands wait
	 * on the fence.
	 */

	sync_method = egl_color_buffer_get_early_display(surf->writeback_color_buffer);

	/* If save_to_file_enabled we work synchronously */
	if (EGL_BUFFER_SYNC_TIMELINE == sync_method && eglp_save_to_file_enabled() == MALI_FALSE)
	{
		platform_fence_type fence = egl_color_buffer_get_fence(surf->writeback_color_buffer);
		/* If we can use the buffer directly, the winsys will not give us a fence */
		if (fence >= 0)
		{
			eglp_block_frame_manager(thread_data->gles_ctx->sync_cmd_queue, dpy,
			                         surf->writeback_color_buffer->dependency_tracker, fence);
			egl_color_buffer_set_fence(surf->writeback_color_buffer, INVALID_PLATFORM_FENCE);
		}
	}
#endif

finish:
	surf->is_native_window_valid = retval;

	return retval;
}

static mali_bool eglp_handle_buffer_prerotate_and_y_inversion(eglp_surface *surf)
{
	mali_bool retval = MALI_TRUE;
	eglp_thread_state *thread_data = eglp_get_current_thread_state();

	if (NULL == thread_data)
	{
		retval = MALI_FALSE;
		goto finish;
	}

	if (NULL == surf->current_color_buffer)
	{
		retval = MALI_FALSE;
		goto finish;
	}

#if MALI_USE_GLES
	if (NULL != thread_data->gles_ctx)
	{
		mali_error res;
		egl_color_buffer_rotation rotation;
		mali_bool y_inversion;

		/* Handle any pre-rotation on the buffer */
		rotation = egl_color_buffer_get_rotation(surf->current_color_buffer);

		/* And y-inversion */
		y_inversion = egl_color_buffer_get_y_inversion(surf->current_color_buffer);

		if (surf == thread_data->gles_draw_surface)
		{
			res = gles_context_set_pre_rotation(thread_data->gles_ctx->client_context.gles_ctx, GLES_DRAW_FBO, rotation,
			                                    y_inversion);
			if (mali_error_is_error(res))
			{
				retval = MALI_FALSE;
				goto finish;
			}
		}

		if (surf == thread_data->gles_read_surface)
		{
			res = gles_context_set_pre_rotation(thread_data->gles_ctx->client_context.gles_ctx, GLES_READ_FBO, rotation,
			                                    y_inversion);
			if (mali_error_is_error(res))
			{
				retval = MALI_FALSE;
				goto finish;
			}
		}
	}
#endif

finish:
	surf->is_native_window_valid = retval;

	return retval;
}

/* This function is not thread safe, it's only supposed to be called from eglSwapBuffers and from
   GLES callback itself (not eglQuerySurface though) */
static void eglp_window_surface_update_client_callback(eglp_thread_state *thread_data, eglp_surface *surf,
                                                       mali_error (*drawcall_cb)(void *), void *key)
{
	CDBG_ASSERT_POINTER(thread_data);
#if MALI_USE_GLES
	if (NULL != thread_data->gles_ctx)
	{
		if (surf == thread_data->gles_draw_surface)
		{
			gles_context_set_first_operation_callback(thread_data->gles_ctx->client_context.gles_ctx, GLES_DRAW_FBO,
			                                          drawcall_cb, key);
		}
		if (surf == thread_data->gles_read_surface)
		{
			gles_context_set_first_operation_callback(thread_data->gles_ctx->client_context.gles_ctx, GLES_READ_FBO,
			                                          drawcall_cb, key);
		}
	}
#endif
}

mali_error eglp_first_operation_cb(void *user_data)
{
	eglp_surface *surf = user_data;
	eglp_thread_state *thread_data = eglp_get_current_thread_state();
	mali_error err = MALI_ERROR_NONE;
	CDBG_ASSERT_POINTER(thread_data);
	CDBG_ASSERT_POINTER(surf);

	/* This function should be called by window surfaces only. */
	CDBG_ASSERT(EGL_WINDOW_BIT == surf->type);

	osu_mutex_lock(&surf->current_color_buffer_lock);
	{
		if (NULL == surf->current_color_buffer)
		{
			err = eglp_window_next_render_target(surf);
		}
	}
	osu_mutex_unlock(&surf->current_color_buffer_lock);

	if (!mali_error_is_error(err))
	{
		if (MALI_FALSE == eglp_handle_buffer_sync_mode(surf))
		{
			err = MALI_ERROR_OUT_OF_MEMORY;
		}
	}

	/* Unregister the callback so it's not called again until after the next swap */
	surf->first_operation_occurred = MALI_TRUE;
	eglp_window_surface_update_client_callback(thread_data, surf, NULL, NULL);

	return err;
}

mali_error eglp_window_next_render_target(eglp_surface *surf)
{
	CDBG_ASSERT_POINTER(surf);
	mali_error retval = MALI_ERROR_NONE;
	eglp_thread_state *thread_data = eglp_get_current_thread_state();
	mali_bool err = MALI_FALSE;
	eglp_display *dpy = surf->dpy;

	CSTD_UNUSED(thread_data);
	CDBG_ASSERT_POINTER(thread_data);

	/* Get next render target from platform layer. The winsys layer will have retained the buffer for us
	 * and we'll release that reference when the surface is destroyed, or when a render to that surface completes.
	 */
	CDBG_ASSERT_POINTER(dpy->winsys->get_window_target_buffer);

	cinstr_trace_tl_block_winsys_get_buffer();

	surf->winsys_error =
	    dpy->winsys->get_window_target_buffer(surf->dpy->winsys_data, surf->winsys_data, &surf->writeback_color_buffer);
	cinstr_trace_tl_nblock_winsys_get_buffer();

	if (NULL == surf->writeback_color_buffer)
	{
		retval = MALI_ERROR_OUT_OF_MEMORY;
		goto finish;
	}

	/* Handle buffer age reporting and tracking.
	 *
	 * We set the age of the current buffer to 1 early. Logically it should be set to 1 once the frame has been
	 * rendered and has been displayed. However with the various early display methods we have, the frame can be
	 * displayed either before or after the next render target has been obtained, and can be displayed from a
	 * different thread to that in which the next render target is obtained. Additionally, the same buffer can in
	 * theory be queued up more than once in the DDK. Such a buffer therefore has multiple ages, depending on which
	 * time it is being used. Updating ages asynchronously is therefore very complicated or impossible.
	 *
	 * To avoid any such complications, we can set the age to 1 here under the assumption that the buffer will get
	 * displayed and in order with previous buffers. After the buffer age has been set to 1 here, it is not strictly
	 * correct as far as the application is concerned. However, by the time the next internal operation on buffer
	 * ages is done, it will be correct. Because we store the correct age in the surface here, the application does
	 * always see the correct age.
	 */
	surf->buffer_age = egl_color_buffer_get_age(surf->writeback_color_buffer);
	egl_color_buffer_set_age(surf->writeback_color_buffer, 1);

	surf->current_color_buffer = surf->writeback_color_buffer;

	err = eglp_window_set_render_target(surf);
	if (MALI_FALSE == err)
	{
		retval = MALI_ERROR_OUT_OF_MEMORY;
		goto finish;
	}

	err = eglp_handle_buffer_prerotate_and_y_inversion(surf);
	if (MALI_FALSE == err)
	{
		retval = MALI_ERROR_OUT_OF_MEMORY;
		goto finish;
	}

#if MALI_USE_GLES
	/* Notify GLES that properties of the surface assigned to the context (e.g. color target) have changed.
	 */

	if (NULL != thread_data->gles_ctx)
	{
		if (surf == thread_data->gles_draw_surface)
		{
			gles_context_surface_changed(thread_data->gles_ctx->client_context.gles_ctx, GLES_DRAW_FBO);
		}

		if (surf == thread_data->gles_read_surface)
		{
			gles_context_surface_changed(thread_data->gles_ctx->client_context.gles_ctx, GLES_READ_FBO);
		}
	}
#endif

finish:
	if (mali_error_is_error(retval))
	{
		if (NULL != surf->writeback_color_buffer)
		{
			/* Buffer content is undefined at this point, so set the age to 0 */
			egl_color_buffer_set_age(surf->writeback_color_buffer, 0);

			/* failures after the buffer is acquired need to cancel the render target
			 * to avoid the buffer being stuck under CPU control */
			if (NULL != dpy->winsys->cancel_render_target)
			{
				CDBG_PRINT_INFO(CDBG_EGL, "Failed - Discarding render target");
				dpy->winsys->cancel_render_target(dpy->winsys_data, surf->winsys_data, surf->writeback_color_buffer);
			}

			egl_color_buffer_release(surf->writeback_color_buffer);
			egl_color_buffer_release(surf->intermediate_color_buffer);

			surf->current_color_buffer = NULL;
			surf->writeback_color_buffer = NULL;
			surf->intermediate_color_buffer = NULL;
		}
	}

	return retval;
}

EGLint *eglp_new_window_surface_default_attribs_array(cmem_hmem_heap_allocator *heap)
{
	EGLint *default_attribs = NULL;

	default_attribs = cmem_hmem_heap_alloc(heap, sizeof(eglp_window_surface_default_attribs));

	if (NULL == default_attribs)
	{
		goto out;
	}

	STDLIB_MEMCPY(default_attribs, eglp_window_surface_default_attribs, sizeof(eglp_window_surface_default_attribs));

out:
	return default_attribs;
}

EGLSurface eglCreateWindowSurface(EGLDisplay display, EGLConfig config, EGLNativeWindowType win,
                                  const EGLint *attrib_list)
{
	return eglp_create_surface(display, config, (void *)(uintptr_t)win, attrib_list, EGL_WINDOW_BIT);
}

EGLSurface eglCreatePlatformWindowSurfaceEXT(EGLDisplay display, EGLConfig config, void *platform_window,
                                             const EGLint *attrib_list)
{
	void *native_window;
	eglp_display *dpy = (eglp_display *)display;

	if (dpy->winsys->platform_get_window)
	{
		native_window = dpy->winsys->platform_get_window(platform_window);
	}
	else
	{
		native_window = platform_window;
	}

	return eglp_create_surface(dpy, config, native_window, attrib_list, EGL_WINDOW_BIT);
}

/** Perform initialization specific only to Window surface.
 *
 * This function is meant to be called from a common function for creating surfaces. Window surface specific
 * initialization will be performed. If anything fails an error will be returned and everything that was
 * succesfully initialized to the point of a fail will be deinitialized so it shouldn't be done also outside this
 * function.
 *
 * @param[in] dpy The display passed to common surface creation function
 * @param[in] surface The window surface to be initialized
 * @param[in] cfg The config passed to common surface creation function
 * @param[in] win The native window passed to common surface creation function
 * @param[in] validated_attribs Should contain all attributes which are valid to be passed to eglCreateWindowSurface().
 *            Attributes which were not passed by the application should be set to default values in this array.
 *
 * @return EGL_SUCCESS in case of a success or other appropriate egl error otherwise.
 */
EGLint eglp_window_surface_specific_initialization(eglp_display *dpy, eglp_surface *surface, eglp_config *cfg,
                                                   void *win, EGLint *validated_attribs)
{
	EGLint error = EGL_SUCCESS;
	osu_errcode buffer_displayed_sem_err = !OSU_ERR_OK;
	osu_errcode buffer_displayed_lock_err = !OSU_ERR_OK;
	osu_errcode current_color_buffer_lock_err = !OSU_ERR_OK;
	osu_errcode swap_started_sem_err = !OSU_ERR_OK;
	EGLBoolean secure = EGL_FALSE;

	surface->is_native_window_valid = MALI_TRUE;
	surface->winsys_data = NULL;
	surface->swap_behavior = EGL_BUFFER_DESTROYED;
	surface->swap_interval = EGLP_DEFAULT_SWAP_INTERVAL;
	surface->wait_buffer_displayed = MALI_FALSE;
	surface->num_buffers_to_display = 0;

	/* Clamp the default swap interval */
	CDBG_ASSERT_LEQ_S(0, cfg->secondary_attribs.min_swap_interval);
	if ((u32)cfg->secondary_attribs.min_swap_interval > surface->swap_interval)
	{
		surface->swap_interval = cfg->secondary_attribs.min_swap_interval;
	}
	CDBG_ASSERT_LEQ_S(0, cfg->secondary_attribs.max_swap_interval);
	if ((u32)cfg->secondary_attribs.max_swap_interval < surface->swap_interval)
	{
		surface->swap_interval = cfg->secondary_attribs.max_swap_interval;
	}

	/* Surfaces are EGL_BACK_BUFFER until a successful transition via eglMakeCurrent or eglSwapBuffers */
	surface->render_buffer = EGL_BACK_BUFFER;
	surface->requested_render_buffer = eglp_get_attribute_value(validated_attribs, EGL_RENDER_BUFFER);

	buffer_displayed_sem_err = osu_sem_init(&surface->buffer_displayed_sem, 0);
	if (OSU_ERR_OK != buffer_displayed_sem_err)
	{
		error = EGL_BAD_ALLOC;
		goto finish;
	}

	buffer_displayed_lock_err =
	    osu_spinlock_init(&surface->buffer_displayed_lock, OSU_LOCK_ORDER_EGL_DISPLAYED_BUFFER_COUNT);
	if (OSU_ERR_OK != buffer_displayed_lock_err)
	{
		error = EGL_BAD_ALLOC;
		goto finish;
	}

	current_color_buffer_lock_err =
	    osu_mutex_init(&surface->current_color_buffer_lock, OSU_LOCK_ORDER_EGL_CURRENT_COLOR_BUFFER);
	if (OSU_ERR_OK != current_color_buffer_lock_err)
	{
		error = EGL_BAD_ALLOC;
		goto finish;
	}

	swap_started_sem_err = osu_sem_init(&surface->swap_started_sem, 0);
	if (OSU_ERR_OK != swap_started_sem_err)
	{
		error = EGL_BAD_ALLOC;
		goto finish;
	}

	/* Check to see if this native window already has a surface associated with it, and error out if so */
	{
		eglp_surface *it;
		mali_bool found = MALI_FALSE;

		CUTILS_DLIST_FOREACH(&dpy->surfaces, eglp_surface, link, it)
		{
			if (it->native_resource == win)
			{
				found = MALI_TRUE;
			}
		}

		if (found != MALI_FALSE)
		{
			error = EGL_BAD_ALLOC;
			goto finish;
		}
		else
		{
			surface->native_resource = win;
		}
	}

#if EGL_USE_PROTECTED_SURFACE
	secure = eglp_get_attribute_value(validated_attribs, EGL_PROTECTED_CONTENT_EXT);
#endif

	CDBG_ASSERT(NULL != dpy->winsys->new_window_surface);
	error = dpy->winsys->new_window_surface(dpy->winsys_data, win, surface, (EGLConfig)cfg, &surface->winsys_data,
	                                        &surface->format, secure);

	if (error != EGL_SUCCESS)
	{
		surface->winsys_data = NULL;
		goto finish;
	}

	if (cobj_surface_format_is_yuv((cobj_surface_format *)&surface->format))
	{
		surface->format = get_intermediate_format(surface->format);
	}

	eglp_color_buffer_format_set_srgb(&surface->format, surface->is_srgb);

finish:
	if (EGL_SUCCESS != error)
	{
		/* An error has occured so we need to clean up any successful allocations */
		if (OSU_ERR_OK == buffer_displayed_sem_err)
		{
			osu_sem_term(&surface->buffer_displayed_sem);
		}
		if (OSU_ERR_OK == buffer_displayed_lock_err)
		{
			osu_spinlock_term(&surface->buffer_displayed_lock);
		}
		if (OSU_ERR_OK == current_color_buffer_lock_err)
		{
			osu_mutex_term(&surface->current_color_buffer_lock);
		}
		if (OSU_ERR_OK == swap_started_sem_err)
		{
			osu_sem_term(&surface->swap_started_sem);
		}
		if (NULL != surface->winsys_data)
		{
			CDBG_ASSERT(NULL != dpy->winsys->delete_surface);
			dpy->winsys->delete_surface(dpy->winsys_data, surface->winsys_data);
			surface->winsys_data = NULL;
		}
	}

	return error;
}

void eglp_window_surface_specific_deinitialization(eglp_surface *surface)
{
	eglp_display *dpy = surface->dpy;
	osu_sem_term(&surface->buffer_displayed_sem);
	osu_spinlock_term(&surface->buffer_displayed_lock);
	osu_mutex_term(&surface->current_color_buffer_lock);
	osu_sem_term(&surface->swap_started_sem);

	CDBG_ASSERT(NULL != dpy->winsys->delete_surface);
	dpy->winsys->delete_surface(dpy->winsys_data, surface->winsys_data);
	surface->winsys_data = NULL;
}

void eglp_display_frame(eglp_window_callback_data *callback_data, egl_buffer_sync_method method)
{
	EGLBoolean res = EGL_FALSE;

	if (eglp_save_to_file_enabled())
	{
		method = EGL_BUFFER_SYNCHRONOUS;
	}

	switch (method)
	{
	case EGL_BUFFER_SYNC_TIMELINE:
	{
		base_fence *fence = NULL;
		platform_fence_type native_fence;

		CDBG_ASSERT(callback_data->sync);

		fence = eglp_sync_get_base_fence(callback_data->sync);
		CDBG_ASSERT(fence);

		native_fence = base_fence_export(fence);
		CDBG_ASSERT(native_fence >= 0);

		egl_color_buffer_set_fence(callback_data->color_buffer, native_fence);

		/* base_fence_export has dup'd the fence from the sync so we can release the sync now */
		eglp_sync_release(callback_data->sync);
		callback_data->sync = NULL;
	}
	break;
	default:
		/* Do nothing. Only here to avoid compiler warnings. */
		break;
	}

/* TODO: MIDEGL-957 */
#if 1 == CSTD_OS_ANDROID
	/* Ensure that the buffer has actually been unlocked before displaying it. If the frame involved
	 * no drawcalls then the DDK will not have already blocked on this lock
	 */
	eglp_color_buffer_wait_unlock(callback_data->color_buffer);

#endif

#if MALI_CINSTR_FEATURE_AVAILABLE_JOB_DUMP
	cinstr_trace_jd_dump_frame_nr(callback_data->frame_nr);
#endif
	EGLBoolean shared_mode = callback_data->render_mode == EGLP_CB_DATA_RENDER_MODE_FF || callback_data->render_mode == EGLP_CB_DATA_RENDER_MODE_FB;
	/* Inform winsys that buffer has changed */
	if (shared_mode)
	{
		if (callback_data->display->winsys->update_shared_buffer)
		{
			EGLBoolean pending_exit = (callback_data->render_mode == EGLP_CB_DATA_RENDER_MODE_FB);
			callback_data->display->winsys->update_shared_buffer(callback_data->surface->winsys_data,
			                                                     callback_data->color_buffer, pending_exit);
			res = EGL_TRUE;
		}
	}
	else
	{
		EGLint *rect_ptr = callback_data->damage_data.rect_count > 0 ? (EGLint *)callback_data->damage_data.rect : NULL;
		EGLint rect_count = callback_data->damage_data.rect_count;

		/* Tell platform layer to display the buffer */
		CDBG_ASSERT_POINTER(callback_data->display->winsys->display_window_buffer);
		res = callback_data->display->winsys->display_window_buffer(callback_data->display->winsys_data,
		                                                            callback_data->surface->winsys_data,
		                                                            callback_data->color_buffer, rect_count, rect_ptr);
	}
	/* If display_window_buffer returns EGL_FALSE this means native window is no longer valid */
	if (EGL_FALSE == res)
	{
		callback_data->surface->is_native_window_valid = MALI_FALSE;
	}
	else
	{
		callback_data->frame_displayed = EGL_TRUE;
	}

	osu_sem_post(&callback_data->frame_displayed_sem);
}

/* Invoked whenever a CMAR command (submitted from EGL) is submitted to Base */
void eglp_cmar_cmd_submitted_callback(void *user_data)
{
	osu_sem *sem = user_data;

	osu_sem_post(sem);
}

/* Enabling CMAR_META_FEATURE_EGL enables the callback above to be called
 * when command has been submitted to base. */
static void eglp_frame_queued_callback(cmar_command_queue *queue, cmar_dependency_list *dep_list,
                                       cmar_metadata_list *metadata_list, void *user_data)
{
	CSTD_UNUSED(queue);
	CSTD_UNUSED(dep_list);

	cmar_metadata_list_set(metadata_list, CMAR_META_FEATURE_EGL, user_data);
}

void eglp_frame_complete_callback(cmar_event *event, cmar_event_status status, void *user_data)
{
	eglp_window_callback_data *callback_data = (eglp_window_callback_data *)user_data;

	/* We're only interested in the event complete or error states */
	if (CMAR_EVENT_COMPLETE < status)
	{
		return;
	}
	else if (CMAR_EVENT_COMPLETE > status)
	{
		CDBG_PRINT_WARN(CDBG_EGL, "Frame completed with an error: %d. Displaying anyway", status);
	}

	CDBG_ASSERT_POINTER(callback_data);
	CDBG_ASSERT_POINTER(callback_data->surface);

	if (eglp_save_to_file_enabled())
	{
		eglp_write_buffer_to_file(callback_data->color_buffer);
	}

	CDBG_PRINT_INFO(CDBG_EGL, "Frame complete");

	{
		egl_buffer_sync_method method;
		method = egl_color_buffer_get_early_display(callback_data->color_buffer);

		/* If save_to_file_enabled we work synchronously */
		if (method == EGL_BUFFER_SYNCHRONOUS || eglp_save_to_file_enabled())
		{
			eglp_display_frame(callback_data, method);

			/* If the winsys is not threadsafe eglSwapBuffers may be waiting for the swap_started_sem */
			EGLBoolean not_threadsafe = egl_color_buffer_get_non_thread_safe(callback_data->color_buffer);
			if (EGL_TRUE == not_threadsafe)
			{
				osu_sem_post(&callback_data->surface->swap_started_sem);
			}
		}
		if (callback_data->render_mode == EGLP_CB_DATA_RENDER_MODE_BF)
		{
			eglp_display_frame(callback_data, EGL_BUFFER_SYNCHRONOUS);
			osu_sem_post(&callback_data->surface->swap_started_sem);
		}
	}

	/* At this point we can be sure that the frame has been displayed as, for 'early buffer display' we are registering
	 * frame complete callback after call to display_frame and for synchronous case display_frame was called above.
	 */
	osu_sem_post(&callback_data->surface->frames_in_flight_sem);

	/* In front buffer rendering mode, we do not perform get_window_target_buffer. So we do not release
	 * the buffer we retained.
	 */
	EGLBoolean retain_buffer = (callback_data->render_mode == EGLP_CB_DATA_RENDER_MODE_FF);
	if (EGL_TRUE == callback_data->frame_displayed)
	{
		egl_window_buffer_displayed(callback_data->color_buffer, callback_data->display,
					    callback_data->surface, retain_buffer);
	}
	else
	{
		if (retain_buffer == EGL_FALSE)
		{
			/* Release color buffer which was retained in get_window_target_buffer().
			 * Always release color buffer before release surface which could unblock display termination.
			 * It may hit segfault in later color buffer destruction if display is terminated beforehead.*/
			egl_color_buffer_release(callback_data->color_buffer);
		}

		/* The surface was retained in eglSwapBuffers(), let's release it. */
		eglp_surface_release(callback_data->surface);
	}

#if MALI_CINSTR_FEATURE_AVAILABLE_HWC_PERFRAME || MALI_CINSTR_FEATURE_AVAILABLE_JOB_DUMP || \
    MALI_CINSTR_FEATURE_AVAILABLE_BUSLOGGER_CONTROL
	{
		CINSTR_END_FRAME(callback_data->surface_id, callback_data->frame_nr);
	}
#endif /* MALI_CINSTR_FEATURE_AVAILABLE_HWC_PERFRAME || MALI_CINSTR_FEATURE_AVAILABLE_JOB_DUMP || MALI_CINSTR_FEATURE_AVAILABLE_BUSLOGGER_CONTROL */

	/* gles_context_flush_surface retained the event which this is a callback of, so we release that reference here. It
	 * has to be released before display_window_buffer is called which could cause eglMakeCurrent to wake up and
	 * terminate everything which could leak the event
	 */
	cmar_release_event(event);
	cutils_refcount_release(&callback_data->refcount);
}

/* Draws all of buff into the current frame (used to implement EGL_BUFFER_PRESERVED swap behavior) */
static mali_bool draw_entire_buffer(eglp_display *dpy, eglp_surface *surf, egl_color_buffer *src, int rotation,
                                    mali_bool y_flip)
{
	eglp_thread_state *thread_data = eglp_get_current_thread_state();
	cobj_surface_template *src_surf_templ = NULL;
	cobj_surface_instance *src_surf_inst = NULL;
	u32 src_width, src_height;
	cdeps_tracker *src_deps;
	mali_bool retval = MALI_FALSE;

	CDBG_ASSERT_POINTER(src);

	src_surf_templ = eglp_color_buffer_get_surface_template(src);
	CDBG_ASSERT_POINTER(src_surf_templ);

	src_surf_inst = cobj_surface_template_get_current_instance(src_surf_templ);
	/* According to COBJ's documentation this call cannot fail, so we can assert the pointer is
	 * not NULL
	 */
	CDBG_ASSERT_POINTER(src_surf_inst);
	src_deps = eglp_color_buffer_get_dependency_tracker(src);
	CDBG_ASSERT_POINTER(src_deps);

	src_width = cobj_surface_instance_get_width(src_surf_inst);
	src_height = cobj_surface_instance_get_height(src_surf_inst);

	/* Draw the whole buffer into the new buffer */
	mali_error draw_err = MALI_ERROR_NONE;
	cutils_rectangle dst_rect;
	f32 coord[8];

	dst_rect.start_x = 0;
	dst_rect.start_y = 0;
	dst_rect.end_x = src_width;
	dst_rect.end_y = src_height;

	/* Take rectangle covering whole frame, GLES defines order in which vertices are evaluated (LB, LT, RB, RT)
	 * with the origin at bottom-left corner. */
	switch (rotation)
	{
	case 0:
		if (y_flip)
		{
			coord[0] = 0;
			coord[1] = src_height;
			coord[2] = 0;
			coord[3] = 0;
			coord[4] = src_width;
			coord[5] = src_height;
			coord[6] = src_width;
			coord[7] = 0;
		}
		else
		{
			coord[0] = 0;
			coord[1] = 0;
			coord[2] = 0;
			coord[3] = src_height;
			coord[4] = src_width;
			coord[5] = 0;
			coord[6] = src_width;
			coord[7] = src_height;
		}
		break;
	case 90:
		dst_rect.end_x = src_height;
		dst_rect.end_y = src_width;
		if (y_flip)
		{
			coord[0] = src_width;
			coord[1] = src_height;
			coord[2] = 0;
			coord[3] = src_height;
			coord[4] = src_width;
			coord[5] = 0;
			coord[6] = 0;
			coord[7] = 0;
		}
		else
		{
			coord[0] = src_width;
			coord[1] = 0;
			coord[2] = 0;
			coord[3] = 0;
			coord[4] = src_width;
			coord[5] = src_height;
			coord[6] = 0;
			coord[7] = src_height;
		}
		break;
	case 180:
		if (y_flip)
		{
			coord[0] = src_width;
			coord[1] = 0;
			coord[2] = src_width;
			coord[3] = src_height;
			coord[4] = 0;
			coord[5] = 0;
			coord[6] = 0;
			coord[7] = src_height;
		}
		else
		{
			coord[0] = src_width;
			coord[1] = src_height;
			coord[2] = src_width;
			coord[3] = 0;
			coord[4] = 0;
			coord[5] = src_height;
			coord[6] = 0;
			coord[7] = 0;
		}
		break;
	case 270:
		dst_rect.end_x = src_height;
		dst_rect.end_y = src_width;
		if (y_flip)
		{
			coord[0] = 0;
			coord[1] = 0;
			coord[2] = src_width;
			coord[3] = 0;
			coord[4] = 0;
			coord[5] = src_height;
			coord[6] = src_width;
			coord[7] = src_height;
		}
		else
		{
			coord[0] = 0;
			coord[1] = src_height;
			coord[2] = src_width;
			coord[3] = src_height;
			coord[4] = 0;
			coord[5] = 0;
			coord[6] = src_width;
			coord[7] = 0;
		}
		break;
	}

#if MALI_USE_GLES
	CDBG_ASSERT_POINTER(thread_data->gles_ctx);
	draw_err = gles_context_draw_preserved_color(thread_data->gles_ctx->client_context.gles_ctx, src_surf_inst, src_deps,
	                                            &dst_rect, coord);
	if (mali_error_is_error(draw_err))
	{
		goto finish;
	}
#endif /* MALI_USE_GLES */

	retval = MALI_TRUE;

finish:
	cobj_instance_release(&src_surf_inst->super);
	cobj_template_release(&src_surf_templ->super);

	return retval;
}

static egl_color_buffer_rotation calc_preserved_buf_rotation(egl_color_buffer *old_buf, egl_color_buffer *new_buf)
{
	int old_rot, new_rot, fin_rot;

	old_rot = (int)egl_color_buffer_get_rotation(old_buf);

	new_rot = (int)egl_color_buffer_get_rotation(new_buf);

	/* Translating 90<->270, because of different GLES-EGL rotation handling (CCW vs CW) */
	old_rot = 360 - old_rot;
	new_rot = 360 - new_rot;

	if (old_rot <= new_rot)
	{
		fin_rot = new_rot - old_rot;
	}
	else
	{
		fin_rot = 360 - (old_rot - new_rot);
	}

	return (egl_color_buffer_rotation)fin_rot;
}

#if MALI_SYSTRACE && CSTD_OS_ANDROID
static const char *eglp_trace_string = "M: Frame queued";
#endif

static void eglp_callback_refcount_callback(cutils_refcount *refcount)
{
	eglp_window_callback_data *callback = CONTAINER_OF(refcount, eglp_window_callback_data, refcount);

	if (callback->sync)
	{
		eglp_sync_release(callback->sync);
	}
	osu_sem_term(&callback->cmd_submitted_sem);
	osu_sem_term(&callback->frame_displayed_sem);
	if (CDBG_TRACE_ENABLED())
	{
		CDBG_TRACE_ASYNC_END(eglp_trace_string, callback->cookie);
	}

	if (callback->damage_data.rect)
	{
		cmem_hmem_heap_free(callback->damage_data.rect);
	}

	cmem_hmem_heap_free(callback);
}

#if MALI_NO_MALI
/** @brief Returns color buffer information for the supplied EGL surface.
 *
 * @param[in]  surface      The EGL surface to return the color buffer information for.
 * @param[out] frame_buffer Pointer to store the color buffer's memory pointer.
 * @param[out] length       Pointer to store the color buffer's length.
 *
 * Function doesn't return any result.
 */
MALI_EXPORT void __eglMaliGetBuffer(EGLSurface surface, unsigned char **frame_buffer, size_t *length)
{
	eglp_surface *surf = (eglp_surface *)surface;

	*frame_buffer = NULL;
	*length = 0;

	if (NULL != surf && NULL != surf->dpy->winsys->get_non_ump_buffer)
	{
		egl_color_buffer *buffer = surf->current_color_buffer;
		if (NULL != buffer)
		{
			*frame_buffer = surf->dpy->winsys->get_non_ump_buffer(buffer);
			*length = eglp_color_buffer_get_size(buffer);
		}
	}
}

/** @brief Releases any resources that may have been allocated by the matching call to __eglMaliGetBuffer.
 *
 * @param[in]  surface      The EGL surface as suppied to __eglMaliGetBuffer.
 *
 * Function doesn't return any result.
 */
MALI_EXPORT void __eglMaliUngetBuffer(EGLSurface *surface)
{
	eglp_surface *surf = (eglp_surface *)surface;

	if (NULL != surf && NULL != surf->dpy->winsys->unget_non_ump_buffer && NULL != surf->current_color_buffer)
	{
		surf->dpy->winsys->unget_non_ump_buffer(surf->current_color_buffer);
	}
}
#endif /* MALI_NO_MALI */

/*
 * @brief Copy damage rectangles from application and store them in the
 * swap callback data structure. Rectangles partially outside the surface
 * boundaries are clamped, and ones completely outside are completely
 * rejected. This serves two purposes: to avoid passing around useless
 * data and also to handle rectangles that have possible become invalid
 * due to surface resize.
 *
 * To avoid wasting memory, the maximum number of "blindly copied"
 * rectangles is 512; if the application passes in more rectangles,
 * we might have to do max. log2(N-1)-8 allocations + copies on the
 * way (where N is the number of good/accepted rectangles).
 *
 * @param heap              Heap allocator to use
 * @param damage_data       Damage data object to populate with
 *                          rectangles
 * @param rects             Pointer to rectangle coordinates
 *                          (4*rect_count ints)
 * @param rect_count        Number of rectangles
 * @param buffer_width      Width to clamp to
 * @param buffer_height     Height to clamp to
 *
 * @return EGL_TRUE if the operation was successful (callback->damage_data
 * struct contents are valid), EGL_FALSE otherwise (OOM)
 */
EGLBoolean eglp_copy_and_clamp_damage_rectangles(cmem_hmem_heap_allocator *heap, eglp_swap_damage_data *damage_data,
                                                 const EGLint *rects, const EGLint rect_count,
                                                 const EGLint buffer_width, const EGLint buffer_height)

{
	EGLint capacity = CSTD_MIN(512, CSTD_MAX(1, rect_count));
	EGLBoolean whole_surface_damage = EGL_FALSE;

	CDBG_ASSERT_POINTER(heap);
	CDBG_ASSERT_POINTER(damage_data);
	CDBG_ASSERT(NULL == damage_data->rect);
	CDBG_ASSERT(buffer_width >= 0);
	CDBG_ASSERT(buffer_width >= 0);
	CDBG_ASSERT(rect_count >= 0);

	if (0 == rect_count)
	{
		whole_surface_damage = EGL_TRUE;
	}
	else if (rect_count > 0 && NULL != rects)
	{
		whole_surface_damage = EGL_FALSE;
	}
	else
	{
		/* Anything else is considered error */
		return EGL_FALSE;
	}

	if (whole_surface_damage)
	{
		/* mali_egl_winsys.h: rect_count = 1, and rects is NULL means full screen damage */
		damage_data->rect = NULL;
		damage_data->rect_count = 1;
	}
	else
	{
		EGLint in_idx, out_idx = 0;
		const EGLint *in_rect = rects;
		eglp_damage_rect *clamped_rects = NULL;

		clamped_rects = (eglp_damage_rect *)cmem_hmem_heap_alloc(heap, sizeof(eglp_damage_rect) * capacity);

		if (NULL == clamped_rects)
		{
			return EGL_FALSE;
		}

		for (in_idx = 0; in_idx < rect_count; in_idx++, in_rect += 4)
		{
			eglp_damage_rect *clamped = clamped_rects + out_idx;
			EGLint right, bottom;

			clamped->x = in_rect[0];
			clamped->y = in_rect[1];
			clamped->width = in_rect[2];
			clamped->height = in_rect[3];

			/* invalid size or completely outside the surface limits */
			if (clamped->width < 0 || clamped->height < 0 || clamped->x >= buffer_width ||
			    clamped->y >= buffer_height || clamped->x + clamped->width <= 0 || clamped->y + clamped->height <= 0)
			{
				continue;
			}

			/* clamp to surface bounds */
			right = clamped->x + clamped->width;
			bottom = clamped->y + clamped->height;

			clamped->x = CSTD_MAX(0, clamped->x);
			clamped->y = CSTD_MAX(0, clamped->y);
			clamped->width = CSTD_MIN(right, buffer_width) - clamped->x;
			clamped->height = CSTD_MIN(bottom, buffer_height) - clamped->y;

			if (0 == clamped->width || 0 == clamped->height)
			{
				continue;
			}

			out_idx++;

			/* enlarge output array if needed. if the allocation fails, keep
			 * what we currently have, exit the loop and consider the operation
			 * a success (since the dirty rectangles are hints anyway, losing
			 * some of them should not break anything). */
			if (out_idx >= capacity && in_idx < rect_count - 1)
			{
				EGLint new_capacity = 2 * capacity;
				eglp_damage_rect *temp;

				temp = (eglp_damage_rect *)cmem_hmem_heap_alloc(heap, sizeof(eglp_damage_rect) * new_capacity);
				if (NULL == temp)
				{
					break;
				}

				STDLIB_MEMCPY(temp, clamped_rects, out_idx * sizeof(eglp_damage_rect));
				cmem_hmem_heap_free(clamped_rects);
				clamped_rects = temp;

				capacity = new_capacity;
			}
		}

		/* if valid clamped rectangles were emitted, update the damage data object */
		if (out_idx > 0)
		{
			damage_data->rect = clamped_rects;
		}
		else
		{
			cmem_hmem_heap_free(clamped_rects);
		}

		damage_data->rect_count = out_idx;
	}

	return EGL_TRUE;
}

#if MALI_CINSTR_FEATURE_AVAILABLE_HWC_PERFRAME || MALI_CINSTR_FEATURE_AVAILABLE_JOB_DUMP || \
    MALI_CINSTR_FEATURE_AVAILABLE_BUSLOGGER_CONTROL
static eglp_window_callback_data *eglp_callback_data_new(cmem_hmem_heap_allocator *heap, eglp_display *dpy,
                                                         eglp_surface *surf, const EGLint *rects, const EGLint rect_count,
                                                         u32 frame_nr)
#else
static eglp_window_callback_data *eglp_callback_data_new(cmem_hmem_heap_allocator *heap, eglp_display *dpy,
                                                         eglp_surface *surf, const EGLint *rects, const EGLint rect_count)
#endif
{
	eglp_window_callback_data *callback_data = NULL;

	/* Prepare callback for completed event */
	callback_data = cmem_hmem_heap_alloc(heap, sizeof(*callback_data));
	if (NULL == callback_data)
	{
		goto finish;
	}

	STDLIB_MEMSET(callback_data, 0, sizeof(*callback_data));

	if (OSU_ERR_OK != osu_sem_init(&callback_data->cmd_submitted_sem, 0))
	{
		goto cleanup1;
	}

	if (OSU_ERR_OK != osu_sem_init(&callback_data->frame_displayed_sem, 0))
	{
		goto cleanup2;
	}

	/*
	 * There are no more possible failures so at this point we initialise the refcount.
	 */
	cutils_refcount_init(&callback_data->refcount, eglp_callback_refcount_callback);

	/* Pass the EGL Color Buffer reference to the callback - going to be released in frame complete callback */
	callback_data->color_buffer = surf->current_color_buffer;
	callback_data->display = dpy;
	callback_data->surface = surf;
	if (CDBG_TRACE_ENABLED())
	{
#if MALI_USE_GLES
		callback_data->cookie = gles_context_get_cookie(dpy->common_ctx);
#endif
		CDBG_TRACE_ASYNC_BEGIN(eglp_trace_string, callback_data->cookie);
	}

#if MALI_CINSTR_FEATURE_AVAILABLE_HWC_PERFRAME || MALI_CINSTR_FEATURE_AVAILABLE_JOB_DUMP || \
    MALI_CINSTR_FEATURE_AVAILABLE_BUSLOGGER_CONTROL
	callback_data->frame_nr = frame_nr;
	callback_data->surface_id = (u64)(uintptr_t)surf;
#endif
	callback_data->frame_displayed = EGL_FALSE;
	/* Copy the damage rectangles over clamping them on the way. This will also handle
	 * the case of zero input damage rectangles and populate the output with single
	 * rectangle that represents whole surface damage */
	if (EGL_TRUE != eglp_copy_and_clamp_damage_rectangles(&dpy->common_ctx->default_heap, &callback_data->damage_data,
	                                                      rects, rect_count, surf->current_color_buffer->width,
	                                                      surf->current_color_buffer->height))
	{
		CDBG_PRINT_WARN(CDBG_EGL, "Failed to copy damage rectangles. Using full-surface damage.");

		CDBG_ASSERT(NULL == callback_data->damage_data.rect);
		CDBG_ASSERT(0 == callback_data->damage_data.rect_count);
	}

	goto finish;

cleanup2:
	osu_sem_term(&callback_data->cmd_submitted_sem);
cleanup1:
	cmem_hmem_heap_free(callback_data);
	callback_data = NULL;

finish:
	return callback_data;
}

static mali_bool eglp_handle_different_writeback(eglp_surface *surf)
{
	eglp_thread_state *thread_data = eglp_get_current_thread_state();
	mali_bool retval = MALI_TRUE;
	mali_error err = MALI_ERROR_NONE;

	egl_color_buffer *new_render_target = NULL;
	cdeps_tracker *deps_tracker = NULL;
	cobj_surface_template *surf_templ = NULL;

	CDBG_ASSERT_POINTER(surf);

	osu_mutex_lock(&surf->current_color_buffer_lock);
	{
		/* If no draw happened, we won't have any buffer to swap, so get one. */
		/* MIDEGL-2700: This happens whether there's a different writeback or not! */
		/* MIDEGL-2700: This also sets the new render target, when it might be overwritten just after this */
		if (surf->current_color_buffer == NULL)
		{
			err = eglp_window_next_render_target(surf);
		}

		if (!mali_error_is_error(err))
		{
			CDBG_ASSERT_POINTER(surf->current_color_buffer);

			if (surf->current_color_buffer != surf->writeback_color_buffer)
			{
				CDBG_ASSERT(surf->current_color_buffer == surf->intermediate_color_buffer);

				surf->current_color_buffer = surf->writeback_color_buffer;
				new_render_target = surf->writeback_color_buffer;
			}
		}
		else
		{
			retval = MALI_FALSE;
		}
	}
	osu_mutex_unlock(&surf->current_color_buffer_lock);

#if MALI_USE_GLES
	/* If we actually have a different render target, set it in GLES. */
	if (NULL != new_render_target)
	{
		deps_tracker = eglp_color_buffer_get_dependency_tracker(new_render_target);
		surf_templ = eglp_color_buffer_get_surface_template(new_render_target);
		CDBG_ASSERT_POINTER(surf_templ);
		CDBG_ASSERT_POINTER(thread_data->gles_ctx);

		err = gles_context_replace_color_render_target(thread_data->gles_ctx->client_context.gles_ctx, surf_templ,
		                                               deps_tracker);

		CDBG_ASSERT((MALI_ERROR_OUT_OF_GPU_MEMORY == err) || (MALI_ERROR_OUT_OF_MEMORY == err) ||
		            (!mali_error_is_error(err)));
		if (mali_error_is_error(err))
		{
			retval = MALI_FALSE;
			goto finish;
		}
	}
#endif /* MALI_USE_GLES */

finish:
	if (NULL != surf_templ)
	{
		cobj_template_release(&surf_templ->super);
	}

	return retval;
}

/* Flush the frame manager - either during eglSwapBuffers or a gles flush callback */
static mali_error eglp_flush_frame_manager(eglp_window_callback_data *callback_data, eglp_thread_state *thread_data,
                                           mali_bool discard_on_finish, cmar_event **frame_complete_event)
{
	CDBG_ASSERT_POINTER(callback_data);
	CDBG_ASSERT_POINTER(thread_data);

	eglp_display *dpy = callback_data->display;
	eglp_surface *surf = callback_data->surface;
	egl_buffer_sync_method sync_method;
	mali_error err_res = MALI_ERROR_NONE;
	mali_bool pending_shared_entry = ((surf->render_buffer == EGL_BACK_BUFFER) &&
					  (surf->requested_render_buffer == EGL_SINGLE_BUFFER));

	if (eglp_save_to_file_enabled() || pending_shared_entry)
	{
		sync_method = EGL_BUFFER_SYNCHRONOUS;
	}
	else
	{
		sync_method = egl_color_buffer_get_early_display(surf->current_color_buffer);
	}

#if MALI_USE_GLES
	CDBG_TRACE_BEGIN("M: Flushing");
	/* Flush the surface */
	CDBG_ASSERT_POINTER(thread_data->gles_ctx);
	switch (sync_method)
	{
	case EGL_BUFFER_SYNCHRONOUS:
		err_res = gles_context_flush_surface(thread_data->gles_ctx->client_context.gles_ctx, NULL, frame_complete_event,
		                                     discard_on_finish);
		break;
	case EGL_BUFFER_SYNC_KDS:
		/* The frame queued callback will be called inside this flush. */
		err_res = gles_context_flush_surface_with_callback(
		    thread_data->gles_ctx->client_context.gles_ctx, NULL, frame_complete_event, discard_on_finish,
		    eglp_frame_queued_callback, &callback_data->cmd_submitted_sem);
		break;
	case EGL_BUFFER_SYNC_TIMELINE:
	{
		/* Get the fragment job complete event from GLES and enqueue a fence triggered by
			 * that so we don't need a round-trip to userspace to signal the fence.
			 * The cmd_submitted_sem is posted when the fence has been enqueued. */
		cmar_event *flush_event = NULL;
		err_res = gles_context_flush_surface(thread_data->gles_ctx->client_context.gles_ctx, &flush_event,
		                                     frame_complete_event, discard_on_finish);
		if (!mali_error_is_error(err_res))
		{
			/* We're about to add a sync to the callback data - check there isn't one already */
			CDBG_ASSERT(!callback_data->sync);
			err_res = eglp_append_fence_to_queue(thread_data->gles_ctx->sync_cmd_queue, flush_event,
			                                     dpy->common_ctx->gpu_device, callback_data);
			if (!mali_error_is_error(err_res))
			{
				err_res = cmar_flush(thread_data->gles_ctx->sync_cmd_queue);
			}
			cmar_release_event(flush_event);
		}
		break;
	}
	default:
		CDBG_PRINT_ERROR(CDBG_EGL, "Got unsupported EGL color buffer sync method %d", (int)sync_method);
		break;
	}
	CDBG_TRACE_END();

	if (mali_error_is_error(err_res) && frame_complete_event != NULL)
	{
		cmar_release_event(*frame_complete_event);
		*frame_complete_event = NULL;
	}
#endif /* MALI_USE_GLES */

	return err_res;
}

/* Reset the surface state - used in eglp_swap_buffers and eglp_gles_flush_cb */
static mali_error eglp_surface_state_reset(eglp_surface *surf)
{
#if MALI_USE_GLES
	eglp_thread_state *thread_data = eglp_get_current_thread_state();
#endif
	mali_error err = MALI_ERROR_NONE;

	/* Reset the EGL_KHR_partial_update per-frame state */
	surf->buffer_age_queried_this_frame = MALI_FALSE;
	surf->buffer_damage_set_this_frame = MALI_FALSE;
	/* We should not put the current color buffer to NULL here for shared mode */
	if (surf->render_buffer == EGL_BACK_BUFFER)
	{
		surf->current_color_buffer = NULL;
		surf->writeback_color_buffer = NULL;
	}
#if MALI_USE_GLES
	err = gles_context_reset_regions(thread_data->gles_ctx->client_context.gles_ctx);
#endif
	return err;

}

/* The main gles flush operation
 *
 * This function is called from eglp_swap_buffers and from eglp_gles_flush_cb functions.
 * The main operations in this function are the following :
 *  -> initialize
 *  -> create a new callback_data
 *  -> flush the surface's frame manager
 *  -> reset the surface state
 *  -> display frame
 *  -> set framecomplete callback
 *  -> perform buffer preserve operations
 *  -> clean up and exit
 */
static mali_bool eglp_gles_flush_operation(eglp_surface *surf, EGLint *rects, EGLint rect_count)
{

	eglp_display *dpy = surf->dpy;
	eglp_thread_state *thread_data = eglp_get_current_thread_state();
	mali_error err_res = MALI_ERROR_NONE;
	mali_bool bool_res = MALI_FALSE;
	mali_bool previous_frame_successfully_set = MALI_FALSE;
	eglp_window_callback_data *callback_data = NULL;
	egl_color_buffer *preload_src_buffer = NULL;
	EGLBoolean not_threadsafe = EGL_FALSE;
	cmar_event *frame_complete_event = NULL;
	mali_bool not_shared_mode = (surf->render_buffer == EGL_BACK_BUFFER);
	mali_bool discard_on_finish = (surf->render_buffer == EGL_BACK_BUFFER);
	mali_bool pending_shared_entry = ((surf->render_buffer == EGL_BACK_BUFFER) &&
					  (surf->requested_render_buffer == EGL_SINGLE_BUFFER));

	eglp_surface_retain(surf);
	/* Prepare callback for completed event */
#if (MALI_CINSTR_FEATURE_AVAILABLE_HWC_PERFRAME || MALI_CINSTR_FEATURE_AVAILABLE_JOB_DUMP || \
     MALI_CINSTR_FEATURE_AVAILABLE_BUSLOGGER_CONTROL)
	u32 next_frame_number = CINSTR_INC_FRAME_NR();
	u32 last_frame_number = next_frame_number - 1;

#if MALI_USE_GLES
	gles_context_set_dumping_frame_number(thread_data->gles_ctx->client_context.gles_ctx, last_frame_number);
#endif

	callback_data = eglp_callback_data_new(&dpy->common_ctx->default_heap, dpy, surf, rects, rect_count, last_frame_number);
	callback_data->frame_nr = last_frame_number;
#else
	callback_data = eglp_callback_data_new(&dpy->common_ctx->default_heap, dpy, surf, rects, rect_count);
#endif
	if (NULL == callback_data)
	{
		thread_data->error = EGL_BAD_ALLOC;
		goto finish;
	}

	/* Set the current render state of the callback_data */
	if (surf->render_buffer == EGL_BACK_BUFFER)
	{
		if(surf->requested_render_buffer == EGL_BACK_BUFFER)
		{
			callback_data->render_mode = EGLP_CB_DATA_RENDER_MODE_BB;
		}
		else
		{
			callback_data->render_mode = EGLP_CB_DATA_RENDER_MODE_BF;
		}
	}
	else
	{
		CDBG_ASSERT(surf->render_buffer == EGL_SINGLE_BUFFER);
		if(surf->requested_render_buffer == EGL_SINGLE_BUFFER)
		{
			callback_data->render_mode = EGLP_CB_DATA_RENDER_MODE_FF;
		}
		else
		{
			callback_data->render_mode = EGLP_CB_DATA_RENDER_MODE_FB;
		}

	}
#if MALI_USE_GLES
	if (not_shared_mode)
	{
		gles_context_invalidate(thread_data->gles_ctx->client_context.gles_ctx, CPOM_RT_DEPTH);
		gles_context_invalidate(thread_data->gles_ctx->client_context.gles_ctx, CPOM_RT_STENCIL);
	}
#endif

	/* Flush the surface's frame manager */
	err_res = eglp_flush_frame_manager(callback_data, thread_data, discard_on_finish, &frame_complete_event);
	if (MALI_ERROR_NONE != err_res)
	{
		thread_data->error = EGL_BAD_ALLOC;
		goto finish;
	}

	/* Store current swap behaviour for remainder of function as it can be changed at any time.
	 * Buffer preserve should happen only if the current rendering mode is back buffer mode. */
	mali_bool buffer_preserved = (surf->swap_behavior == EGL_BUFFER_PRESERVED) && not_shared_mode;
	if (buffer_preserved)
	{
		/* Keep a reference on the previous buffer until its content has been drawn into the new buffer */
		preload_src_buffer = surf->current_color_buffer;
		egl_color_buffer_retain(preload_src_buffer);
	}
	err_res = eglp_surface_state_reset(surf);

	CDBG_ASSERT(MALI_ERROR_NONE == err_res);

	/* It is possible the frame complete callback runs in this thread, so increment
	 * num_buffers_to_display before. */
	osu_spinlock_lock(&surf->buffer_displayed_lock);
	surf->num_buffers_to_display++;

	osu_spinlock_unlock(&surf->buffer_displayed_lock);

	egl_buffer_sync_method sync_method;
	if (eglp_save_to_file_enabled() || pending_shared_entry)
	{
		sync_method = EGL_BUFFER_SYNCHRONOUS;
	}
	else
	{
		sync_method = egl_color_buffer_get_early_display(callback_data->color_buffer);
	}

	if (EGL_BUFFER_SYNC_TIMELINE == sync_method || EGL_BUFFER_SYNC_KDS == sync_method)
	{
		/* Wait for the commands to be enqueued to base */
		CDBG_TRACE_BEGIN("M: Waiting for fence enqueue");
		osu_sem_wait(&callback_data->cmd_submitted_sem);
		CDBG_TRACE_END();
		eglp_display_frame(callback_data, sync_method);
		if (MALI_FALSE == surf->is_native_window_valid)
		{
			thread_data->error = EGL_BAD_NATIVE_WINDOW;
			goto finish;
		}
	}
	/* retain callback data for the frame complete callback */
	cutils_refcount_retain(&callback_data->refcount);

	/* get the threadsafe property from the buffer */
	if (EGL_BUFFER_SYNCHRONOUS == sync_method)
	{
		not_threadsafe = egl_color_buffer_get_non_thread_safe(callback_data->color_buffer);
	}
	/* Register the callback for the frame completed event.
	 * The callback may occur at any point after the beginning of this call.
	 * If the event has already occurred the callback is called in this thread during this call. */
	err_res =
	    cmar_set_event_callback(frame_complete_event, eglp_frame_complete_callback, callback_data, CMAR_EVENT_COMPLETE);
	if (MALI_ERROR_NONE != err_res)
	{
		cutils_refcount_release(&callback_data->refcount); /* release here as callback is not going to happen */
		thread_data->error = EGL_BAD_ALLOC;
		osu_spinlock_lock(&surf->buffer_displayed_lock);
		CDBG_ASSERT_MSG(surf->num_buffers_to_display != 0, "num_buffers_to_display decremented incorrectly");
		surf->num_buffers_to_display--;
		osu_spinlock_unlock(&surf->buffer_displayed_lock);
		goto finish;
	}
	previous_frame_successfully_set = MALI_TRUE;


	/* The frame complete callback may have been called during cmar_set_event_callback() so check for a failure */
	if (MALI_FALSE == surf->is_native_window_valid)
	{
		thread_data->error = EGL_BAD_ALLOC;
		goto finish;
	}

	CDBG_TRACE_BEGIN("M: Work enqueue block");
	osu_sem_wait(&surf->frames_in_flight_sem);
	CDBG_TRACE_END();
	if (not_shared_mode)
	{
		/* Register the callback for the first drawcall on the surface */
		surf->first_operation_occurred = MALI_FALSE;
		eglp_window_surface_update_client_callback(thread_data, surf, eglp_first_operation_cb, surf);

		/* If we're not threadsafe we wait for the swap_started_sem before continuing */
		if (not_threadsafe || pending_shared_entry)
		{
			osu_sem_wait(&surf->swap_started_sem);

			/* now we know we have called display_window_buffer() so we are okay to call get_window_target_buffer()
			 * after this point */
		}
	}
	if (buffer_preserved)
	{
		/* We need to ensure that target buffer is ready to use */
		err_res = eglp_first_operation_cb(surf);
		if (MALI_ERROR_NONE != err_res)
		{
			if (surf->winsys_error == EGL_BAD_NATIVE_WINDOW)
			{
				/* eglSwapBuffers takes an EGLSurface, not an EGLNativeWindowType,
				 * so we return EGL_BAD_SURFACE instead of EGL_BAD_NATIVE_WINDOW.
				 * cf. '3.1 Errors' from the EGL specification */
				thread_data->error = EGL_BAD_SURFACE;
			}
			else
			{
				thread_data->error = EGL_BAD_ALLOC;
			}
			goto finish;
		}

		int rotation = calc_preserved_buf_rotation(preload_src_buffer, callback_data->color_buffer);
		mali_bool y_inversion = egl_color_buffer_get_y_inversion(callback_data->color_buffer) !=
		                        egl_color_buffer_get_y_inversion(preload_src_buffer);

		bool_res = draw_entire_buffer(dpy, surf, preload_src_buffer, rotation, y_inversion);
		if (bool_res == MALI_FALSE)
		{
			thread_data->error = EGL_BAD_ALLOC;
			goto finish;
		}

		/* We stored the actual buffer age in the surface when we obtained the render target earlier, and set
		 * the buffer age to 1 assuming it would get displayed in future. However because we are now preserving
		 * the buffer, the effective age that the application queries also needs to be 1, because it now
		 * contains the same content as the back buffer for the previous frame.
		 */
		/* MIDEGL-2703: Need to check this works with the YUV intermediate buffer / writeback buffer stuff. See
		 * MIDEGL-2318 for similar issue. */
		CDBG_ASSERT(egl_color_buffer_get_age(surf->current_color_buffer) == 1);
		surf->buffer_age = 1;
	}

	if (eglp_save_to_file_enabled() && MALI_TRUE == previous_frame_successfully_set)
	{
		osu_sem_wait(&callback_data->frame_displayed_sem);
	}
	bool_res = MALI_TRUE;
finish:
	if (NULL != preload_src_buffer)
	{
		egl_color_buffer_release(preload_src_buffer);
	}
	if (NULL != callback_data)
	{
		cutils_refcount_release(&callback_data->refcount);
	}
	if (EGL_FALSE == bool_res)
	{
		if (MALI_FALSE == previous_frame_successfully_set)
		{
			/* If don't get to this block this means the surface will be released either in
			 * eglp_frame_complete_callback() or in egl_window_buffer_displayed().
			 */
			eglp_surface_release(surf);
			cmar_release_event(frame_complete_event);
		}
	}
	return bool_res;
}

/* Callback function to handle a flush request from GLES
 *
 * Called from GLES when a buffer is in shared mode. This callback retains surface and the color buffer, then
 * calls the gles flush operation.
 *
 * This must only be called from API functions that are explicitly defined to flush (because it will trigger a
 * recomposition which could result in whole geometry "tearing" when the application isn't expecting it.)
 * Incremental renders and any other "internal" flushes are allowed to write to the shared buffer (subject to any
 * fences) but must not call into the window system.
 *
 * key is a pointer to the surface being flushed.
 */
static mali_error eglp_gles_flush_cb(void *key)
{
	eglp_surface *surf = (eglp_surface *)key;
	CDBG_PRINT_INFO(CDBG_EGL, "gles shared buffer flush");

	mali_bool bool_res = eglp_gles_flush_operation(surf, NULL, 0);
	if (bool_res == MALI_TRUE)
	{
#if (MALI_CINSTR_FEATURE_AVAILABLE_HWC_PERFRAME || MALI_CINSTR_FEATURE_AVAILABLE_JOB_DUMP || \
     MALI_CINSTR_FEATURE_AVAILABLE_BUSLOGGER_CONTROL) && MALI_USE_GLES
		eglp_thread_state *thread_data = eglp_get_current_thread_state();
		u32 next_frame_number = CINSTR_GET_FRAME_NR();
		CINSTR_START_FRAME((u64)(uintptr_t)surf, next_frame_number);
		cinstr_trace_jd_dump_frame_nr(next_frame_number);
		if (thread_data->gles_ctx != NULL)
		{
			gles_context_set_dumping_frame_number(thread_data->gles_ctx->client_context.gles_ctx,
							      next_frame_number);
		}
#endif /* MALI_CINSTR_FEATURE_AVAILABLE_HWC_PERFRAME || MALI_CINSTR_FEATURE_AVAILABLE_JOB_DUMP || MALI_CINSTR_FEATURE_AVAILABLE_BUSLOGGER_CONTROL \
          && MALI_USE_GLES */
		return MALI_ERROR_NONE;
	}
	else
	{
		CDBG_PRINT_WARN(CDBG_EGL, "gles shared buffer flush failed");
		return MALI_ERROR_OUT_OF_GPU_MEMORY;
	}
}

/* Function for actually performing eglSwapBuffer
 *
 * This function is called from the entry points eglSwapBuffers, eglSwapBuffersWithDamageEXT and
 * eglSwapBuffersWithDamageKHR. The logic in this function is organized into sections laid out as following.
 *
 * We have 4 states : FF, FB, BB, BF
 * FF = Front buffer mode currently and front buffer mode requested
 * FB = Front buffer mode currently and Back buffer mode requested and so on.
 *
 * First operation is sanity checking the current state and we retain the display and surface.
 *
 * FF - return from function - this will also be hit if the surface is not a window surface.
 *
 * We perform an initial setup by setting up intermediate color buffer and call the first draw call callback.
 *
 * FB - transition + perform gles flush operation - cleanup + return
 *
 * Perform the gles flush operation (here it is for both BB and BF states).
 *
 * BF - transition
 *
 * Clean up and return
 */
MALI_EXPORT EGLBoolean eglp_swap_buffers(EGLDisplay display, EGLSurface surface, EGLint *rects, EGLint rect_count)
{
	EGLBoolean retval = EGL_FALSE;
	eglp_display *dpy = (eglp_display *)display;
	eglp_surface *surf = (eglp_surface *)surface;
	eglp_thread_state *thread_data = eglp_get_current_thread_state();
	mali_error err_res = MALI_ERROR_NONE;
	mali_bool bool_res;
	mali_bool surface_retained = MALI_FALSE;
	mali_bool display_retained = MALI_FALSE;
	cmar_event *frame_complete_event = NULL;

	cinstr_trace_tl_call_egl_swapbuffers();

#if (MALI_CINSTR_FEATURE_AVAILABLE_HWC_PERFRAME || MALI_CINSTR_FEATURE_AVAILABLE_JOB_DUMP || \
     MALI_CINSTR_FEATURE_AVAILABLE_BUSLOGGER_CONTROL) && MALI_USE_GLES
	u32 next_frame_number = 0;
#endif /* MALI_CINSTR_FEATURE_AVAILABLE_HWC_PERFRAME|| MALI_CINSTR_FEATURE_AVAILABLE_JOB_DUMP || MALI_CINSTR_FEATURE_AVAILABLE_BUSLOGGER_CONTROL */
	if (NULL == thread_data)
	{
		goto finish;
	}

	if (rect_count < 0 || (rect_count > 0 && NULL == rects))
	{
		thread_data->error = EGL_BAD_PARAMETER;
		goto finish;
	}

	thread_data->error = eglp_check_display_valid_and_initialized_and_retain(dpy);
	if (EGL_SUCCESS != thread_data->error)
	{
		goto finish;
	}
	display_retained = MALI_TRUE;

	thread_data->error = eglp_check_surface_valid_and_retain(dpy, surf);
	if (EGL_SUCCESS != thread_data->error)
	{
		goto finish;
	}
	surface_retained = MALI_TRUE;

	/* Check that surface is current for the current rendering API */
	if (surf != thread_data->gles_draw_surface)
	{
		thread_data->error = EGL_BAD_SURFACE;
		goto finish;
	}

	/*  FF : Check for conditions that exit without effect and are not errors :
	 * - The surface is not a window surface.
	 * - The surface is single-buffered and there is no requested change to the EGL_RENDER_BUFFER attribute.
	 */
	if ((EGL_WINDOW_BIT != surf->type) ||
	    ((surf->render_buffer == EGL_SINGLE_BUFFER) && (surf->requested_render_buffer != EGL_BACK_BUFFER)))
	{
		/* We release the references we took during the validation step */
		eglp_surface_release(surf);
		eglp_display_release(dpy);

		/* Return early directly here to simplify the cleanup code later as this is a successful call but
		 * doesn't require any cleanup */
		return EGL_TRUE;
	}

#if MALI_USE_GLES
#if MALI_GLES_QA
	gles_config_dump_sw_counters(thread_data->gles_ctx->client_context.gles_ctx, GLES_QA_DUMP_EVENT_FRAME_FINISHING);
#endif

	/* If we are using an intermediate color buffer (for YUV for example).
	 * Switch to the correct one. */
	bool_res = eglp_handle_different_writeback(surf);
	if (MALI_FALSE == bool_res)
	{
		if (surf->winsys_error == EGL_BAD_NATIVE_WINDOW)
		{
			/* eglSwapBuffers takes an EGLSurface, not an EGLNativeWindowType,
			 * so we return EGL_BAD_SURFACE instead of EGL_BAD_NATIVE_WINDOW.
			 * cf. '3.1 Errors' from the EGL specification */
			thread_data->error = EGL_BAD_SURFACE;
		}
		else
		{
			thread_data->error = EGL_BAD_ALLOC;
		}

		goto finish;
	}

	/* calls the first draw call callback */
	bool_res = gles_context_flush(thread_data->gles_ctx->client_context.gles_ctx, MALI_TRUE);
	if (MALI_FALSE == bool_res)
	{
		thread_data->error = EGL_BAD_ALLOC;
		goto finish;
	}
#else
	CDBG_ASSERT_MSG(MALI_FALSE, "OpenGL ES not supported");
#endif
#if MALI_USE_GLES
	/* FB : Transition from Shared Buffer mode to Back Buffer mode */
	if ((surf->render_buffer == EGL_SINGLE_BUFFER) && (surf->requested_render_buffer == EGL_BACK_BUFFER))
	{
		/* Note : There is no back buffer to preserve so we don't care about EGL_BUFFER_DESTROYED or _PRESERVED */
		CDBG_PRINT_INFO(CDBG_EGL, "Transition Shared->Back Buffer mode");

		CDBG_ASSERT(NULL != dpy->winsys->set_shared_buffer_mode);
		/* done before flush & update to ensure that update transitions out of shared mode */
		EGLBoolean mode_changed = dpy->winsys->set_shared_buffer_mode(surf->winsys_data, EGL_FALSE);
		if (!mode_changed)
		{
			CDBG_PRINT_WARN(CDBG_EGL, "Change to back buffer mode failed!");
			/* drop the requested state & return an error */
			surf->requested_render_buffer = EGL_SINGLE_BUFFER;
			thread_data->error = EGL_BAD_ALLOC;
		}
		else
		{
			/* call the gles flush callback function to perform the flush & update the winsys buffer */
			bool_res = eglp_gles_flush_operation(surf, NULL, 0);
			if (bool_res == MALI_FALSE)
			{
				thread_data->error = EGL_BAD_ALLOC;
				goto finish;
			}

			/* disable gles shared mode callback and release its user data */
			void *old_callback_data;
			gles_context_disable_shared_mode_callback(thread_data->gles_ctx->client_context.gles_ctx,
			                                          &old_callback_data);

			/* Reset the color buffers as they are not reset in shared mode */
			surf->current_color_buffer = NULL;
			surf->writeback_color_buffer = NULL;

			/* Register the first operation callback */
			surf->first_operation_occurred = MALI_FALSE;
			eglp_window_surface_update_client_callback(thread_data, surf, eglp_first_operation_cb, surf);

			/* Mark the transition */
			surf->render_buffer = EGL_BACK_BUFFER;
			retval = EGL_TRUE;
		}
		goto finish;
	}
#endif /* MALI_USE_GLES */

	bool_res = eglp_gles_flush_operation(surf, rects, rect_count);
	if (bool_res == MALI_FALSE)
	{
		thread_data->error = EGL_BAD_ALLOC;
		goto finish;
	}
#if MALI_USE_GLES && defined(EGL_KHR_mutable_render_buffer) && EGL_KHR_mutable_render_buffer
	/*
	 * BF : Handle a transition into shared buffer mode
	 */
	if ((surf->render_buffer == EGL_BACK_BUFFER) && (surf->requested_render_buffer == EGL_SINGLE_BUFFER) &&
	    (surf->config->secondary_attribs.surface_type & EGL_MUTABLE_RENDER_BUFFER_BIT_KHR))
	{
		CDBG_PRINT_INFO(CDBG_EGL, "Transition Back->Shared Buffer mode");

		CDBG_ASSERT(NULL != dpy->winsys->set_shared_buffer_mode);
		EGLBoolean mode_changed = dpy->winsys->set_shared_buffer_mode(surf->winsys_data, EGL_TRUE);

		if (!mode_changed)
		{
			CDBG_PRINT_WARN(CDBG_EGL, "Change to shared buffer mode failed!");
			/* drop requested state */
			surf->requested_render_buffer = EGL_BACK_BUFFER;
			thread_data->error = EGL_BAD_ALLOC;
			goto finish;
		}
		else
		{
			/* register & enable the callback from GLES */
			gles_context_enable_shared_mode_callback(thread_data->gles_ctx->client_context.gles_ctx, eglp_gles_flush_cb,
			                                         (void *)surf);

			/* mark transitioned */
			surf->render_buffer = EGL_SINGLE_BUFFER;
		}
	}
#endif

	retval = EGL_TRUE;

finish:
#if (MALI_CINSTR_FEATURE_AVAILABLE_HWC_PERFRAME || MALI_CINSTR_FEATURE_AVAILABLE_JOB_DUMP || \
     MALI_CINSTR_FEATURE_AVAILABLE_BUSLOGGER_CONTROL) && MALI_USE_GLES
	next_frame_number = CINSTR_GET_FRAME_NR();
	if (EGL_TRUE == retval)
	{
		CINSTR_START_FRAME((u64)(uintptr_t)surf, next_frame_number);
		cinstr_trace_jd_dump_frame_nr(next_frame_number);
	}
#endif

	if (MALI_TRUE == surface_retained)
	{
		egl_surface_release(surf);
	}

	if (MALI_TRUE == display_retained)
	{
		eglp_display_release(dpy);
	}

#if (MALI_CINSTR_FEATURE_AVAILABLE_HWC_PERFRAME || MALI_CINSTR_FEATURE_AVAILABLE_JOB_DUMP || \
     MALI_CINSTR_FEATURE_AVAILABLE_BUSLOGGER_CONTROL) &&                                     \
    MALI_USE_GLES
	if (thread_data->gles_ctx != NULL)
	{
		gles_context_set_dumping_frame_number(thread_data->gles_ctx->client_context.gles_ctx, next_frame_number);
	}
#endif /* (MALI_CINSTR_FEATURE_AVAILABLE_HWC_PERFRAME || MALI_CINSTR_FEATURE_AVAILABLE_JOB_DUMP || MALI_CINSTR_FEATURE_AVAILABLE_BUSLOGGER_CONTROL) \
           && MALI_USE_GLES */

	/* Everything has already been cleaned up if there was an error */
	cinstr_trace_tl_ncall_egl_swapbuffers();
	return retval;
}

EGLBoolean eglSwapBuffers(EGLDisplay display, EGLSurface surface)
{
	return eglp_swap_buffers(display, surface, NULL, 0);
}

EGLBoolean eglSwapBuffersWithDamageEXT(EGLDisplay display, EGLSurface surface, EGLint *rects, EGLint n_rects)
{
	return eglp_swap_buffers(display, surface, rects, n_rects);
}

EGLBoolean eglSwapBuffersWithDamageKHR(EGLDisplay display, EGLSurface surface, EGLint *rects, EGLint n_rects)
{
	return eglp_swap_buffers(display, surface, rects, n_rects);
}

MALI_IMPL EGLBoolean eglSetDamageRegionKHR(EGLDisplay display, EGLSurface surface, EGLint *rects, EGLint nrects)
{
	EGLBoolean retval = EGL_FALSE;
	eglp_display *dpy = (eglp_display *)display;
	eglp_surface *surf = (eglp_surface *)surface;
	eglp_thread_state *thread_data = eglp_get_current_thread_state();
	cmem_hmem_heap_allocator *heap;
	egl_color_buffer *current_color_buffer = NULL;
	mali_bool surface_retained = MALI_FALSE;
	mali_bool display_retained = MALI_FALSE;
	cutils_rectangle *rects_for_gles = NULL;
	cutils_rectangle *this_rect_for_gles;
	size_t non_empty_rects = 0;
	EGLint buf_width, buf_height, surface_width, surface_height;
	int i;
	egl_color_buffer_rotation rotation;
	mali_bool whole_surface_damage = MALI_FALSE;

	if (NULL == thread_data)
	{
		goto finish;
	}

	if (nrects < 0)
	{
		/* Not strictly in the spec, but a negative number of rects doesn't
		 * make sense.
		 */
		thread_data->error = EGL_BAD_PARAMETER;
		goto finish;
	}

	thread_data->error = eglp_check_display_valid_and_initialized_and_retain(dpy);
	if (EGL_SUCCESS != thread_data->error)
	{
		goto finish;
	}
	display_retained = MALI_TRUE;

	heap = &dpy->common_ctx->default_heap;

	thread_data->error = eglp_check_surface_valid_and_retain(dpy, surf);
	if (EGL_SUCCESS != thread_data->error)
	{
		goto finish;
	}
	surface_retained = MALI_TRUE;

	if (surf != thread_data->gles_draw_surface)
	{
		thread_data->error = EGL_BAD_MATCH;
		goto finish;
	}
	if (surf->type != EGL_WINDOW_BIT)
	{
		thread_data->error = EGL_BAD_MATCH;
		goto finish;
	}

	if (surf->swap_behavior != EGL_BUFFER_DESTROYED)
	{
		thread_data->error = EGL_BAD_MATCH;
		goto finish;
	}

	if (surf->buffer_age_queried_this_frame == MALI_FALSE)
	{
		thread_data->error = EGL_BAD_ACCESS;
		goto finish;
	}

	if (surf->buffer_damage_set_this_frame == MALI_TRUE)
	{
		thread_data->error = EGL_BAD_ACCESS;
		goto finish;
	}

	current_color_buffer = surf->current_color_buffer;

	/* At this point we're guaranteed that the age has been queried, and to query the age requires the current color
	 * buffer to have been retrieved.
	 */
	CDBG_ASSERT_POINTER(current_color_buffer);

	/* Surface_width and Surface_height are the width and height of the surface as
	 * returned from eglQuerySurface(EGL_WIDTH/EGL_HEIGHT), they are the dimension
	 * that the passed rectangles are relative to.
	 * while the buf_width and buf_height is the real(rotated) width/height which the
	 * passed in rectangels need to be transformed into. */
	surface_width = buf_width = egl_color_buffer_get_width(current_color_buffer);
	surface_height = buf_height = egl_color_buffer_get_height(current_color_buffer);
	rotation = egl_color_buffer_get_rotation(current_color_buffer);
	if (rotation == EGL_COLOR_BUFFER_ROTATION_90 || rotation == EGL_COLOR_BUFFER_ROTATION_270)
	{
		surface_width = buf_height;
		surface_height = buf_width;
	}

	/* Need to convert into cutils_rectangles for GLES. Allocate assuming no rectangles are empty, we may pass
	 * less than this number through to GLES if some rectangles are empty (non_empty_rects).
	 */
	rects_for_gles = cmem_hmem_heap_alloc(heap, nrects * 4 * sizeof(*rects_for_gles));
	if (rects_for_gles == NULL)
	{
		thread_data->error = EGL_BAD_ALLOC;
		goto finish;
	}

	/* For each rectangle:
	 * - Discard if empty
	 * - Clamp to surface dimensions
	 * - Invert co-ordinates from bottom-left origin to top-left origin
	 * - Convert to cutils_rectangle representation
	 */

	this_rect_for_gles = rects_for_gles;

	for (i = 0; i < nrects; i++)
	{
		/* The rectangle in the eglSetDamageRegionKHR co-ordinate system, with the origin in the lower left
		 * corner */
		EGLint egl_x = rects[i * 4 + 0];
		EGLint egl_y = rects[i * 4 + 1];
		EGLint egl_w = rects[i * 4 + 2];
		EGLint egl_h = rects[i * 4 + 3];

		/* Discard any rectangles completely outside of the surface in positive direction */
		if (egl_x >= surface_width || egl_y >= surface_height)
		{
			continue;
		}

		/* Clamp rectangles to at least zero x and y */
		if (egl_x < 0)
		{
			egl_w += egl_x; /* Adding a negative - subtraction overall */
			egl_x = 0;
		}
		if (egl_y < 0)
		{
			egl_h += egl_y; /* Adding a negative - subtraction overall */
			egl_y = 0;
		}

		if (egl_w <= 0 || egl_h <= 0)
		{
			continue;
		}

		/* Clamp rectangles to not extend past surface_width and surface_height */
		if (egl_x + egl_w > surface_width)
		{
			egl_w -= (egl_x + egl_w) - surface_width;
			CDBG_ASSERT(egl_w > 0);
		}
		if (egl_y + egl_h > surface_height)
		{
			egl_h -= (egl_y + egl_h) - surface_height;
			CDBG_ASSERT(egl_h > 0);
		}

		if (egl_w == 0 || egl_h == 0)
		{
			continue;
		}

		CDBG_ASSERT(egl_w > 0);
		CDBG_ASSERT(egl_h > 0);

		/* If the damage region covers the whole surface, quit processing the
		 * remaining rectangles and just call gles_context_reset_regions(),
		 * instead, avoiding the overhead from creating the stencil buffer. */
		if (egl_x == 0 && egl_y == 0 && egl_w >= surface_width && egl_h >= surface_height)
		{
			whole_surface_damage = MALI_TRUE;
			break;
		}

		/* Adjust x, y according to rotation */
		switch (rotation)
		{
		/* EGL is using CW rotation */
		case EGL_COLOR_BUFFER_ROTATION_0:
			break;
		case EGL_COLOR_BUFFER_ROTATION_90:
		{
			int temp;
			temp = egl_w;
			egl_w = egl_h;
			egl_h = temp;

			temp = egl_x;
			egl_x = egl_y;
			egl_y = buf_height - temp - egl_h;
		}
		break;
		case EGL_COLOR_BUFFER_ROTATION_180:
		{
			egl_x = buf_width - egl_x - egl_w;
			egl_y = buf_height - egl_y - egl_h;
		}
		break;
		case EGL_COLOR_BUFFER_ROTATION_270:
		{
			int temp;

			temp = egl_w;
			egl_w = egl_h;
			egl_h = temp;

			temp = egl_x;
			egl_x = buf_width - egl_y - egl_w;
			egl_y = temp;
		}
		break;
		default:
			CDBG_ASSERT_MSG(MALI_FALSE, "Unsupported rotation!\n");
			break;
		}

		CDBG_ASSERT(egl_x >= 0);
		this_rect_for_gles->start_x = egl_x;
		this_rect_for_gles->end_x = egl_x + egl_w - 1;

		if (EGL_TRUE == egl_color_buffer_get_y_inversion(current_color_buffer))
		{

			egl_y = egl_y + egl_h - 1; /* Bottom-left corner of the box -> top-left corner of the box */
			egl_y = buf_height - egl_y - 1; /* GLES coord system -> window coord system */

			/* Window coord system, top-left corner
			   egl_y < egl_y + egl_h - 1
			   Negative stride is not used */
		}
		/* else
		{
			GLES coord system, bottom-left corner
			egl_y < egl_y + egl_h - 1
			Negative stride is used
		} */

		CDBG_ASSERT(egl_y >= 0);
		this_rect_for_gles->start_y = egl_y;
		this_rect_for_gles->end_y = egl_y + egl_h - 1;

		CDBG_ASSERT(this_rect_for_gles->end_x >= this_rect_for_gles->start_x);
		CDBG_ASSERT(this_rect_for_gles->end_y >= this_rect_for_gles->start_y);
		CDBG_ASSERT(buf_width >= 0 && this_rect_for_gles->end_x < (u32)buf_width);
		CDBG_ASSERT(buf_height >= 0 && this_rect_for_gles->end_y < (u32)buf_height);

		non_empty_rects++;
		this_rect_for_gles++;
	}

#if MALI_USE_GLES
	/* Set the regions for any non-zero number of rectangles, even if the actual is zero
	 * (i.e. non_empty_rects == 0). From the spec: nrects == 0 means the damage region is
	 * set to the whole surface, but nrects != 0 && non_empty_rects == 0 means the damage
	 * region is set to the empty region. */
	if (nrects != 0 && !whole_surface_damage)
	{
		mali_error err_res = MALI_ERROR_NONE;

		err_res =
		    gles_context_set_regions(thread_data->gles_ctx->client_context.gles_ctx, non_empty_rects, rects_for_gles);
		if (mali_error_is_error(err_res))
		{
			thread_data->error = EGL_BAD_ALLOC;
			goto finish;
		}
	}
	else
	{
		mali_error err_res = gles_context_reset_regions(thread_data->gles_ctx->client_context.gles_ctx);
		CDBG_ASSERT(!mali_error_is_error(err_res));
		CSTD_UNUSED(err_res);
	}
#endif /* MALI_USE_GLES */

	surf->buffer_damage_set_this_frame = MALI_TRUE;

	retval = EGL_TRUE;

finish:
	if (rects_for_gles != NULL)
	{
		cmem_hmem_heap_free(rects_for_gles);
	}
	if (surface_retained == MALI_TRUE)
	{
		egl_surface_release(surf);
	}
	if (display_retained == MALI_TRUE)
	{
		egl_display_release(dpy);
	}

	return retval;
}

#if MALI_USE_GLES
void eglp_window_surface_enable_shared_mode(eglp_display *dpy, eglp_surface *surface, gles_context *gles_ctx)
{
#if defined(EGL_KHR_mutable_render_buffer) && EGL_KHR_mutable_render_buffer

	CDBG_ASSERT(EGL_BACK_BUFFER == surface->render_buffer);

	if (surface->config->secondary_attribs.surface_type & EGL_MUTABLE_RENDER_BUFFER_BIT_KHR)
	{
		if (dpy->winsys->set_shared_buffer_mode)
		{
			dpy->winsys->set_shared_buffer_mode(surface->winsys_data, EGL_TRUE);

			gles_context_enable_shared_mode_callback(gles_ctx, eglp_gles_flush_cb, (void *)surface);
			surface->render_buffer = EGL_SINGLE_BUFFER;
		}
	}
#endif /* defined(EGL_KHR_mutable_render_buffer) && EGL_KHR_mutable_render_buffer */
}

void eglp_window_surface_disable_shared_mode(eglp_display *dpy, eglp_surface *surface, gles_context *gles_ctx)
{
	CDBG_ASSERT(EGL_SINGLE_BUFFER == surface->render_buffer);

	if (dpy->winsys->set_shared_buffer_mode)
	{
		dpy->winsys->set_shared_buffer_mode(surface->winsys_data, EGL_FALSE);

		void *callback_data;
		gles_context_disable_shared_mode_callback(gles_ctx, &callback_data);
		CDBG_ASSERT(callback_data == surface);
		CSTD_UNUSED(callback_data);
		surface->render_buffer = EGL_BACK_BUFFER;
	}
}
#endif /* MALI_USE_GLES */
