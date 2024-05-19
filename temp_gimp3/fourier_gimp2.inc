/** Plugin interface *********************************************************/

static void query(void);
static void run(const gchar *name,
                gint nparams,
                const GimpParam *param,
                gint *nreturn_vals,
                GimpParam **return_vals);

GimpPlugInInfo PLUG_IN_INFO = {
    NULL,  /* init_proc  */
    NULL,  /* quit_proc  */
    query, /* query_proc */
    run    /* run_proc   */
};



MAIN()

void query(void)
{
  /* Definition of parameters */
  static GimpParamDef args[] = {
      {GIMP_PDB_INT32, (gchar *)"run_mode", (gchar *)"Interactive, non-interactive"},
      {GIMP_PDB_IMAGE, (gchar *)"image", (gchar *)"Input image (unused)"},
      {GIMP_PDB_DRAWABLE, (gchar *)"drawable", (gchar *)"Input drawable"}};

  /* Forward FFT */
  gimp_install_procedure(
      PLUG_IN_DIR_PROC,
      PLUG_IN_DIR_DESC,
      PLUG_IN_DIR_SHORT_DESC,
      PLUG_IN_AUTHOR,
      PLUG_IN_AUTHOR,
      PLUG_IN_VERSION,
      PLUG_IN_DIR_MENU_LABEL,
      "RGB*, GRAY*",
      GIMP_PLUGIN,
      G_N_ELEMENTS(args), 0,
      args, NULL);
  gimp_plugin_menu_register(PLUG_IN_DIR_PROC, PLUG_IN_MENU_LOCATION);

  /* Inverse FFT */
  gimp_install_procedure(
      PLUG_IN_INV_PROC,
      PLUG_IN_INV_DESC,
      PLUG_IN_INV_SHORT_DESC,
      PLUG_IN_AUTHOR,
      PLUG_IN_AUTHOR,
      PLUG_IN_VERSION,
      PLUG_IN_INV_MENU_LABEL,
      "RGB*, GRAY*",
      GIMP_PLUGIN,
      G_N_ELEMENTS(args), 0,
      args, NULL);
  gimp_plugin_menu_register(PLUG_IN_INV_PROC, PLUG_IN_MENU_LOCATION);
}

static void
run(const gchar *name,
    gint nparams,
    const GimpParam *param,
    gint *nreturn_vals,
    GimpParam **return_vals)
{
  /* Return values */
  static GimpParam values[1];

  gint sel_x1, sel_y1, sel_x2, sel_y2, sel_width, sel_height, padding;
  gint img_height, img_width, img_bpp, img_has_alpha;

  gint32 drawable_id;
  GimpDrawable *drawable;
  GimpRunMode run_mode;
  GimpPDBStatusType status;
  const Babl *format;

  GeglBuffer *buffer;
  GeglRectangle *roi;
  guchar *img_pixels;

  int fft_inv = 0;

  if (strcmp(name, PLUG_IN_INV_PROC) == 0)
  {
    fft_inv = 1;
  }

  *nreturn_vals = 1;
  *return_vals = values;

  status = GIMP_PDB_SUCCESS;

  if (param[0].type != GIMP_PDB_INT32)
    status = GIMP_PDB_CALLING_ERROR;
  if (param[2].type != GIMP_PDB_DRAWABLE)
    status = GIMP_PDB_CALLING_ERROR;

  run_mode = (GimpRunMode)param[0].data.d_int32;

  
  gegl_init (NULL, NULL);

  drawable_id = param[2].data.d_drawable;

  img_width = gimp_drawable_width(drawable_id);
  img_height = gimp_drawable_height(drawable_id);
  // img_bpp = gimp_drawable_get_bpp(drawable_id);
  img_has_alpha = gimp_drawable_has_alpha(drawable_id);

  if (gimp_drawable_has_alpha(drawable_id)) //  gimp_drawable_is_rgb (drawable)
    format = babl_format("R'G'B'A u8");
  else
    format = babl_format("R'G'B' u8");

  img_bpp = babl_format_get_bytes_per_pixel(format);

  gimp_drawable_mask_bounds(drawable_id, &sel_x1, &sel_y1, &sel_x2, &sel_y2);

  // Ensure selection does not exceed image
  if (sel_x1 < 0) sel_x1 = 0; if (sel_x1 > img_width) sel_x1 = img_width;
  if (sel_y1 < 0) sel_y1 = 0; if (sel_y1 > img_height) sel_y1 = img_height;
  if (sel_x2 < 0) sel_x2 = 0; if (sel_x2 > img_width) sel_x2 = img_width;
  if (sel_y2 < 0) sel_y2 = 0; if (sel_y2 > img_height) sel_y2 = img_height;

  sel_width = sel_x2 - sel_x1;
  sel_height = sel_y2 - sel_y1;

  //printf("Image size %dx%d - %d bpp\n", img_width, img_height, img_bpp);
  //printf("Selection size %dx%d (%d,%d-%d,%d)\n", sel_width, sel_height, sel_x1, sel_y1, sel_x2, sel_y2);

  if (status == GIMP_PDB_SUCCESS)
  {
    gimp_progress_init(fft_inv ? "Applying inverse Fourier transform..." : "Applying forward Fourier transform...");

    // Init buffers
    GeglBuffer *src_buffer = gimp_drawable_get_buffer(drawable_id);
    GeglBuffer *dest_buffer = gimp_drawable_get_shadow_buffer(drawable_id);
    
    roi = GEGL_RECTANGLE(sel_x1, sel_y1, sel_width, sel_height);
    img_pixels = g_malloc(roi->width * roi->height * img_bpp);

    // Get source image
    gegl_buffer_get(src_buffer, roi, 1.0, format, img_pixels, GEGL_AUTO_ROWSTRIDE, GEGL_ABYSS_NONE);

    if (fft_inv == 0)
    {
      process_fft_forward(img_pixels, img_pixels, sel_width, sel_height, img_bpp, img_bpp);
    }
    else
    {
      process_fft_inverse(img_pixels, img_pixels, sel_width, sel_height, img_bpp, img_bpp);
    }

    // Set result to image
    gegl_buffer_set(dest_buffer, GEGL_RECTANGLE(sel_x1, sel_y1, sel_x2, sel_y2), 0,
                    format, img_pixels,
                    GEGL_AUTO_ROWSTRIDE);

    g_free(img_pixels);
    g_object_unref(src_buffer);
    g_object_unref(dest_buffer);

    gimp_drawable_merge_shadow(drawable_id, TRUE);
    gimp_drawable_update(drawable_id, sel_x1, sel_y1, (sel_x2 - sel_x1), (sel_y2 - sel_y1));
    gimp_displays_flush();

    // set FG to neutral grey; used to mask moire patterns, etc
    if (fft_inv == 0)
    {
      GimpRGB neutral_grey;
      gimp_rgba_set_uchar(&neutral_grey, 128, 128, 128, 1);
      gimp_context_set_foreground(&neutral_grey);
    }

    gimp_progress_init(fft_inv ? "Inverse Fourier transform applied successfully." : "Forward Fourier transform applied successfully.");

    values[0].type = GIMP_PDB_STATUS;
    values[0].data.d_status = status;
  }
}
