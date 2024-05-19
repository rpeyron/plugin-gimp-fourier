// fourier plugin rewrite based on hot.c bundled GIMP plugin

#include <string.h>

#include <libgimp/gimp.h>
#include <libgimp/gimpui.h>

#define GETTEXT_PACKAGE "glib"

#include "libgimp/stdplugins-intl.h"

#include <fftw3.h>

#if __has_include("fourier-config.h")
#include "fourier-config.h"
#else
#define VERSION "0.4.5"
#endif

/** Defines ******************************************************************/

#define PLUG_IN_NAME "plug_in_fft"
#define PLUG_IN_VERSION "Mar 2024, " VERSION

#define PLUG_IN_PROC "plug-in-fourier"
#define PLUG_IN_BINARY "fourier_gimp3"
#define PLUG_IN_ROLE "gimp-fourier"

#include "fourier_common.inc"

typedef struct _Fourier Fourier;
typedef struct _FourierClass FourierClass;

// GIMP_MAJOR_VERSION                              

struct _Fourier
{
  GimpPlugIn parent_instance;
};

struct _FourierClass
{
  GimpPlugInClass parent_class;
};

#define FOURIER_TYPE (fourier_get_type())
#define FOURIER(obj) (G_TYPE_CHECK_INSTANCE_CAST((obj), FOURIER_TYPE, Fourier))

GType fourier_get_type(void) G_GNUC_CONST;

static GList *fourier_query_procedures(GimpPlugIn *plug_in);
static GimpProcedure *fourier_create_procedure(GimpPlugIn *plug_in,
                                               const gchar *name);

static GimpValueArray *fourier_run(GimpProcedure *procedure,
                                   GimpRunMode run_mode,
                                   GimpImage *image,
                                   gint n_drawables,
                                   GimpDrawable **drawables,
                                   GimpProcedureConfig *config,
                                   gpointer run_data);

static gboolean fourier_core(GimpImage *image,
                           GimpDrawable *drawable,
                           GObject *config);

static gboolean plugin_dialog(GimpProcedure *procedure,
                              GObject *config);

G_DEFINE_TYPE(Fourier, fourier, GIMP_TYPE_PLUG_IN)

GIMP_MAIN(FOURIER_TYPE)
DEFINE_STD_SET_I18N

typedef enum
{
  MODE_FORWARD,
  MODE_INVERSE
} fourierModes;

static void
fourier_class_init(FourierClass *klass)
{
  GimpPlugInClass *plug_in_class = GIMP_PLUG_IN_CLASS(klass);

  plug_in_class->query_procedures = fourier_query_procedures;
  plug_in_class->create_procedure = fourier_create_procedure;
  plug_in_class->set_i18n = STD_SET_I18N;
}

static void
fourier_init(Fourier *fourier)
{
}

static GList *
fourier_query_procedures(GimpPlugIn *plug_in)
{
  return g_list_append(NULL, g_strdup(PLUG_IN_PROC));
}

static GimpProcedure *
fourier_create_procedure(GimpPlugIn *plug_in,
                         const gchar *name)
{
  GimpProcedure *procedure = NULL;

  if (!strcmp(name, PLUG_IN_PROC))
  {
    procedure = gimp_image_procedure_new(plug_in, name,
                                         GIMP_PDB_PROC_TYPE_PLUGIN,
                                         fourier_run, NULL, NULL);

    gimp_procedure_set_image_types(procedure, "RGB");
    gimp_procedure_set_sensitivity_mask(procedure,
                                        GIMP_PROCEDURE_SENSITIVE_DRAWABLE);

    gimp_procedure_set_menu_label(procedure, _("_Fourier..."));
    gimp_procedure_add_menu_path(procedure, "<Image>/Filters/Generic");

    gimp_procedure_set_documentation(procedure,
                                     _("This plug-in applies a FFT to the image, for educationnal or effects purpose."),
                                     "Apply an FFT to the image. This can remove (for example) moire patterns from images scanned from books:\n\n"
                                     "    The image should be RGB (Image|Mode|RGB)\n\n"
                                     "    Remove the alpha layer, if present (Image|Flatten Image)\n\n"
                                     "    Select Filters|Generic|FFT Forward\n\n"
                                     "    Use the preselected neutral grey to effectively remove any moir patterns from the image. Either paint over any patterns or\n\n"
                                     "     - In the Layers window, select the layer, and 'Duplicate Layer'\n"
                                     "     - Select Colours|Brightness-Contrast. Increase the Contrast to see any patterns.\n"
                                     "     - Use the Rectangular and/or Elliptical Selection tools to select any patterns on the contrast layer.\n"
                                     "     - Then remove the contrast layer leaving the original FFT layer with the selections.\n"
                                     "     - Then select Edit|Fill with FG colour, remembering to cancel the Selection afterwards!\n\n"
                                     "    Select Filters|Generic|FFT Inverse\n\n"
                                     "Voila, an image without the moire pattern!",
                                     name);
    gimp_procedure_set_attribution(procedure,
                                   "Remi Peyronnet",
                                   "Remi Peyronnet",
                                   PLUG_IN_VERSION);

    GIMP_PROC_ARG_INT(procedure, "mode",
                      _("Mode"),
                      _("Mode { Foward (0), Inversed (1) }"),
                      0, 1, MODE_FORWARD,
                      G_PARAM_READWRITE);

    GIMP_PROC_ARG_BOOLEAN(procedure, "new-layer",
                          _("Create _new layer"),
                          _("Create a new layer"),
                          TRUE,
                          G_PARAM_READWRITE);
  }

  return procedure;
}

static GimpValueArray *
fourier_run(GimpProcedure *procedure,
            GimpRunMode run_mode,
            GimpImage *image,
            gint n_drawables,
            GimpDrawable **drawables,
            GimpProcedureConfig *config,
            gpointer run_data)
{
  GimpDrawable *drawable;

  gegl_init(NULL, NULL);

  if (n_drawables != 1)
  {
    GError *error = NULL;

    g_set_error(&error, GIMP_PLUG_IN_ERROR, 0,
                _("Procedure '%s' only works with one drawable."),
                gimp_procedure_get_name(procedure));

    return gimp_procedure_new_return_values(procedure,
                                            GIMP_PDB_CALLING_ERROR,
                                            error);
  }
  else
  {
    drawable = drawables[0];
  }

  if (run_mode == GIMP_RUN_INTERACTIVE && !plugin_dialog(procedure, G_OBJECT(config)))
    return gimp_procedure_new_return_values(procedure,
                                            GIMP_PDB_CANCEL,
                                            NULL);

  if (!fourier_core(image, drawable, G_OBJECT(config)))
    return gimp_procedure_new_return_values(procedure,
                                            GIMP_PDB_EXECUTION_ERROR,
                                            NULL);

  if (run_mode != GIMP_RUN_NONINTERACTIVE)
    gimp_displays_flush();

  return gimp_procedure_new_return_values(procedure, GIMP_PDB_SUCCESS, NULL);
}

static gboolean
fourier_core(GimpImage *image,
           GimpDrawable *drawable,
           GObject *config)
{
  gint mode;
  gint action;
  gboolean new_layer;
  GeglBuffer *src_buffer;
  GeglBuffer *dest_buffer;
  const Babl *src_format;
  const Babl *dest_format;
  gint src_bpp;
  gint dest_bpp;
  gboolean success = TRUE;
  GimpLayer *nl = NULL;
  gint y, i;
  gint Y, I, Q;
  gint width, height;
  gint sel_x1, sel_x2, sel_y1, sel_y2;
  gint prog_interval;
  guchar *src, *s, *dst, *d;
  guchar r, prev_r = 0, new_r = 0;
  guchar g, prev_g = 0, new_g = 0;
  guchar b, prev_b = 0, new_b = 0;
  gdouble fy, fc, t, scale;
  gdouble pr, pg, pb;
  gdouble py;

  g_object_get(config,
               "mode", &mode,
               "new-layer", &new_layer,
               NULL);

  width = gimp_drawable_get_width(drawable);
  height = gimp_drawable_get_height(drawable);

  if (gimp_drawable_has_alpha(drawable))
    src_format = babl_format("R'G'B'A u8");
  else
    src_format = babl_format("R'G'B' u8");

  dest_format = src_format;

  if (new_layer)
  {
    gchar name[40];
    const gchar *mode_names[] =
        {
            "forward",
            "inversed",
        };

    g_snprintf(name, sizeof(name), "fourier mask (%s)", mode_names[mode]);

    nl = gimp_layer_new(image, name, width, height,
                        GIMP_RGBA_IMAGE,
                        100,
                        gimp_image_get_default_new_layer_mode(image));

    gimp_drawable_fill(GIMP_DRAWABLE(nl), GIMP_FILL_TRANSPARENT);
    gimp_image_insert_layer(image, nl, NULL, 0);

    dest_format = babl_format("R'G'B'A u8");
  }

  if (!gimp_drawable_mask_intersect(drawable,
                                    &sel_x1, &sel_y1, &width, &height))
    return success;

  src_bpp = babl_format_get_bytes_per_pixel(src_format);
  dest_bpp = babl_format_get_bytes_per_pixel(dest_format);

  sel_x2 = sel_x1 + width;
  sel_y2 = sel_y1 + height;

  src = g_new(guchar, width * height * src_bpp);
  dst = g_new(guchar, width * height * dest_bpp);

  src_buffer = gimp_drawable_get_buffer(drawable);

  if (new_layer)
  {
    dest_buffer = gimp_drawable_get_buffer(GIMP_DRAWABLE(nl));
  }
  else
  {
    dest_buffer = gimp_drawable_get_shadow_buffer(drawable);
  }

  gegl_buffer_get(src_buffer,
                  GEGL_RECTANGLE(sel_x1, sel_y1, width, height), 1.0,
                  src_format, src,
                  GEGL_AUTO_ROWSTRIDE, GEGL_ABYSS_NONE);

  s = src;
  d = dst;

  gimp_progress_init(_("Fourier"));
  prog_interval = height / 10;

  if (mode == 0)
  { // Forward
    process_fft_forward(src, dst, width, height, src_bpp, dest_bpp);
  }
  else
  { // Inverse
    process_fft_inverse(src, dst, width, height, src_bpp, dest_bpp);
  }

  // for (y = sel_y1; y < sel_y2; y++)
  // {
  //   gint x;

  //   if (y % prog_interval == 0)
  //     gimp_progress_update((double)y / (double)(sel_y2 - sel_y1));

  //   for (x = sel_x1; x < sel_x2; x++)
  //   {
  //     if (hotp(r = *(s + 0), g = *(s + 1), b = *(s + 2)))
  //     {
  //       if (action == ACT_FLAG)
  //       {
  //         for (i = 0; i < 3; i++)
  //           *d++ = 0;
  //         s += 3;
  //         if (src_bpp == 4)
  //           *d++ = *s++;
  //         else if (new_layer)
  //           *d++ = 255;
  //       }
  //       else
  //       {
  //         /*
  //          * Optimization: cache the last-computed hot pixel.
  //          */
  //         if (r == prev_r && g == prev_g && b == prev_b)
  //         {
  //           *d++ = new_r;
  //           *d++ = new_g;
  //           *d++ = new_b;
  //           s += 3;
  //           if (src_bpp == 4)
  //             *d++ = *s++;
  //           else if (new_layer)
  //             *d++ = 255;
  //         }
  //         else
  //         {
  //           Y = tab[0][0][r] + tab[0][1][g] + tab[0][2][b];
  //           I = tab[1][0][r] + tab[1][1][g] + tab[1][2][b];
  //           Q = tab[2][0][r] + tab[2][1][g] + tab[2][2][b];

  //           prev_r = r;
  //           prev_g = g;
  //           prev_b = b;
  //           /*
  //            * Get Y and chroma amplitudes in floating point.
  //            *
  //            * If your C library doesn't have hypot(), just use
  //            * hypot(a,b) = sqrt(a*a, b*b);
  //            *
  //            * Then extract linear (un-gamma-corrected)
  //            * floating-point pixel RGB values.
  //            */
  //           fy = (double)Y / (double)SCALE;
  //           fc = hypot((double)I / (double)SCALE,
  //                      (double)Q / (double)SCALE);

  //           pr = (double)pix_decode(r);
  //           pg = (double)pix_decode(g);
  //           pb = (double)pix_decode(b);

  //           /*
  //            * Reducing overall pixel intensity by scaling R,
  //            * G, and B reduces Y, I, and Q by the same factor.
  //            * This changes luminance but not saturation, since
  //            * saturation is determined by the chroma/luminance
  //            * ratio.
  //            *
  //            * On the other hand, by linearly interpolating
  //            * between the original pixel value and a grey
  //            * pixel with the same luminance (R=G=B=Y), we
  //            * change saturation without affecting luminance.
  //            */
  //           if (action == ACT_LREDUX)
  //           {
  //             /*
  //              * Calculate a scale factor that will bring the pixel
  //              * within both chroma and composite limits, if we scale
  //              * luminance and chroma simultaneously.
  //              *
  //              * The calculated chrominance reduction applies
  //              * to the gamma-corrected RGB values that are
  //              * the input to the RGB-to-YIQ operation.
  //              * Multiplying the original un-gamma-corrected
  //              * pixel values by the scaling factor raised to
  //              * the "gamma" power is equivalent, and avoids
  //              * calling gc() and inv_gc() three times each.  */
  //             scale = chroma_lim / fc;
  //             t = compos_lim / (fy + fc);
  //             if (t < scale)
  //               scale = t;
  //             scale = pow(scale, mode_vals[mode].gamma);

  //             r = (guint8)pix_encode(scale * pr);
  //             g = (guint8)pix_encode(scale * pg);
  //             b = (guint8)pix_encode(scale * pb);
  //           }
  //           else
  //           { /* ACT_SREDUX hopefully */
  //             /*
  //              * Calculate a scale factor that will bring the
  //              * pixel within both chroma and composite
  //              * limits, if we scale chroma while leaving
  //              * luminance unchanged.
  //              *
  //              * We have to interpolate gamma-corrected RGB
  //              * values, so we must convert from linear to
  //              * gamma-corrected before interpolation and then
  //              * back to linear afterwards.
  //              */
  //             scale = chroma_lim / fc;
  //             t = (compos_lim - fy) / fc;
  //             if (t < scale)
  //               scale = t;

  //             pr = gc(pr, mode);
  //             pg = gc(pg, mode);
  //             pb = gc(pb, mode);

  //             py = pr * mode_vals[mode].code[0][0] +
  //                  pg * mode_vals[mode].code[0][1] +
  //                  pb * mode_vals[mode].code[0][2];

  //             r = pix_encode(inv_gc(py + scale * (pr - py),
  //                                   mode));
  //             g = pix_encode(inv_gc(py + scale * (pg - py),
  //                                   mode));
  //             b = pix_encode(inv_gc(py + scale * (pb - py),
  //                                   mode));
  //           }

  //           *d++ = new_r = r;
  //           *d++ = new_g = g;
  //           *d++ = new_b = b;

  //           s += 3;

  //           if (src_bpp == 4)
  //             *d++ = *s++;
  //           else if (new_layer)
  //             *d++ = 255;
  //         }
  //       }
  //     }
  //     else
  //     {
  //       if (!new_layer)
  //       {
  //         for (i = 0; i < src_bpp; i++)
  //           *d++ = *s++;
  //       }
  //       else
  //       {
  //         s += src_bpp;
  //         d += dest_bpp;
  //       }
  //     }
  //   }
  // }

  gegl_buffer_set(dest_buffer,
                  GEGL_RECTANGLE(sel_x1, sel_y1, width, height), 0,
                  dest_format, dst,
                  GEGL_AUTO_ROWSTRIDE);

  gimp_progress_update(1.0);

  g_free(src);
  g_free(dst);

  g_object_unref(src_buffer);
  g_object_unref(dest_buffer);

  if (new_layer)
  {
    gimp_drawable_update(GIMP_DRAWABLE(nl), sel_x1, sel_y1, width, height);
  }
  else
  {
    gimp_drawable_merge_shadow(drawable, TRUE);
    gimp_drawable_update(drawable, sel_x1, sel_y1, width, height);
  }

  gimp_displays_flush();

  return success;
}

static gboolean
plugin_dialog(GimpProcedure *procedure,
              GObject *config)
{
  GtkWidget *dlg;
  GtkWidget *vbox;
  GtkWidget *hbox;
  GtkListStore *store;
  gboolean run;

  gimp_ui_init(PLUG_IN_BINARY);

  dlg = gimp_procedure_dialog_new(procedure,
                                  GIMP_PROCEDURE_CONFIG(config),
                                  _("Fourier"));

  gimp_dialog_set_alternative_button_order(GTK_DIALOG(dlg),
                                           GTK_RESPONSE_OK,
                                           GTK_RESPONSE_CANCEL,
                                           -1);

  gimp_window_set_transient(GTK_WINDOW(dlg));

  store = gimp_int_store_new(_("_Forward"), MODE_FORWARD,
                             _("_Inverse"), MODE_INVERSE,
                             NULL);
  gimp_procedure_dialog_get_int_radio(GIMP_PROCEDURE_DIALOG(dlg),
                                      "mode", GIMP_INT_STORE(store));

  vbox = gimp_procedure_dialog_fill_box(GIMP_PROCEDURE_DIALOG(dlg),
                                        "fourier-left-side",
                                        "mode",
                                        "new-layer",
                                        NULL);
  gtk_box_set_spacing(GTK_BOX(vbox), 12);

  hbox = gimp_procedure_dialog_fill_box(GIMP_PROCEDURE_DIALOG(dlg),
                                        "fourier-hbox",
                                        "fourier-left-side",
                                        "action",
                                        NULL);
  gtk_box_set_spacing(GTK_BOX(hbox), 12);
  gtk_box_set_homogeneous(GTK_BOX(hbox), TRUE);
  gtk_widget_set_margin_bottom(hbox, 12);
  gtk_orientable_set_orientation(GTK_ORIENTABLE(hbox),
                                 GTK_ORIENTATION_HORIZONTAL);

  gimp_procedure_dialog_fill(GIMP_PROCEDURE_DIALOG(dlg),
                             "fourier-hbox",
                             NULL);

  gtk_widget_show(dlg);

  run = gimp_procedure_dialog_run(GIMP_PROCEDURE_DIALOG(dlg));

  gtk_widget_destroy(dlg);

  return run;
}
