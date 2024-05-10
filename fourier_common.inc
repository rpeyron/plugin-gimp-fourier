/** Conversion functions *****************************************************/

inline gint round_gint(double value)
{
  double floored = floor(value);
  if (value - floored > 0.5)
  {
    return (gint)(floored + 1);
  }
  return (gint)floored;
}

inline gint boost(double value)
{
  double bounded = fabs(value / 160.0);
  gint boosted = round_gint(128.0 * sqrt(bounded));
  boosted = (value > 0) ? boosted : -boosted;
  return boosted;
}

inline double unboost(double value)
{
  double bounded = fabs(value / 128.0);
  double unboosted = 160.0 * bounded * bounded;
  unboosted = (value > 0) ? unboosted : -unboosted;
  return unboosted;
}

inline guchar get_guchar(gint x, gint y, double d)
{
  gint i = round_gint(d);
  // if (i > 255 || i < 0) { printf(" (%d, %d: %d) ", x, y, i); }
  return (guchar)(i >= 255) ? 255 : ((i < 0) ? 0 : i);
}

inline guchar get_gchar128(gint x, gint y, gint i)
{
  // if (i > 127 || i < -128) { printf(" (%d, %d: %d) ", x, y, i); }
  return (guchar)(i >= (gint)128) ? 255 : ((i <= (gint)-128) ? 0 : i + 128);
}

inline double get_double128(gint x, gint y, guchar c)
{
  return (double)(c)-128.0;
}

/* Should pixel store imaginary part? */
static inline gint pixel_imag(gint row, gint col, gint h, gint w)
{
  if (row == 0 && h % 2 == 0 || row == h / 2)
    return col > w / 2;
  else
    return row > h / 2;
}

/*
 * Map images coordinates (row, col) into Fourier array indices (row2, col2).
 */
static inline void map(gint row, gint col, gint h, gint w,
                       gint *row2, gint *col2)
{
  *row2 = (row + (h + 1) / 2) % h; /* shift origin */
  *col2 = (col + (w + 1) / 2) % w;
  if (*col2 > w / 2)
  { /* wrap */
    *row2 = (h - *row2) % h;
    *col2 = w - *col2;
  }
  *col2 *= 2; /* unit = real number */
  if (pixel_imag(row, col, h, w))
    (*col2)++; /* take imaginary part */
}

inline double normalize(gint x, gint y, gint width, gint height)
{
  double cx = (double)abs(x - width / 2);
  double cy = (double)abs(y - height / 2);
  double energy = (sqrt(cx) + sqrt(cy));
  return energy * energy;
}

/** Process Functions *********************************************************/

void process_fft_forward(guchar *src_pixels, guchar *dst_pixels, gint sel_width, gint sel_height, gint src_bpp, gint dst_bpp)
{

  gint row, col, row2, col2, cur_bpp, bounded, padding;
  gint progress, max_progress;
  fftw_plan p;
  double v, norm;
  double *fft_real;

  padding = (sel_width & 1) ? 1 : 2;

  fft_real = g_new(double, (sel_width + padding) * sel_height);

  progress = 0;
  max_progress = src_bpp * 3;

  p = fftw_plan_dft_r2c_2d(sel_height, sel_width, fft_real, (fftw_complex *)fft_real, FFTW_ESTIMATE);

  for (cur_bpp = 0; cur_bpp < src_bpp; cur_bpp++)
  {
    for (col = 0; col < sel_width; col++)
    {
      for (row = 0; row < sel_height; row++)
      {
        v = (double)src_pixels[(row * sel_width + col) * src_bpp + cur_bpp];
        fft_real[row * (sel_width + padding) + col] = v;
      }
    }
    gimp_progress_update((double)++progress / max_progress);
    fftw_execute(p);
    gimp_progress_update((double)++progress / max_progress);

    for (row = 0; row < sel_height; row++)
    {
      for (col = 0; col < sel_width; col++)
      {
        map(row, col, sel_height, sel_width, &row2, &col2);
        v = fft_real[row2 * (sel_width + padding) + col2] / (double)(sel_width * sel_height);
        norm = normalize(col, row, sel_width, sel_height);
        bounded = boost(v * norm);
        dst_pixels[(row * sel_width + col) * dst_bpp + cur_bpp] = get_gchar128(col, row, bounded);
      }
    }
    // do not boost (0, 0), just offset it
    row = sel_height / 2;
    col = sel_width / 2;
    bounded = round_gint((fft_real[0] / (double)(sel_width * sel_height)) - 128.0);
    dst_pixels[(row * sel_width + col) * dst_bpp + cur_bpp] = get_gchar128(col, row, bounded);
    gimp_progress_update((double)++progress / max_progress);
  }

  fftw_destroy_plan(p);
  g_free(fft_real);
}

void process_fft_inverse(guchar *src_pixels, guchar *dst_pixels, gint sel_width, gint sel_height, gint src_bpp, gint dst_bpp)
{

  gint row, col, row2, col2, cur_bpp, bounded, padding;
  gint progress, max_progress;
  fftw_plan p;
  double v, norm;
  double *fft_real;

  padding = (sel_width & 1) ? 1 : 2;

  fft_real = g_new(double, (sel_width + padding) * sel_height);

  progress = 0;
  max_progress = src_bpp * 3;

  p = fftw_plan_dft_c2r_2d(sel_height, sel_width, (fftw_complex *)fft_real, fft_real, FFTW_ESTIMATE);

  for (cur_bpp = 0; cur_bpp < src_bpp; cur_bpp++)
  {
    for (row = 0; row < sel_height; row++)
    {
      for (col = 0; col < sel_width; col++)
      {
        map(row, col, sel_height, sel_width, &row2, &col2);
        norm = normalize(col, row, sel_width, sel_height);
        v = get_double128(row, col, src_pixels[(row * sel_width + col) * src_bpp + cur_bpp]);
        fft_real[row2 * (sel_width + padding) + col2] = unboost(v) / norm;
      }
    }
    // restore redundancy
    for (col2 = 0; col2 < sel_width + padding; col2 += (sel_width + 1) / 2 * 2)
    {
      for (row2 = 1; row2 < (sel_height + 1) / 2; row2++)
      {
        fft_real[(sel_height - row2) * (sel_width + padding) + col2 + 1] = -fft_real[row2 * (sel_width + padding) + col2 + 1];
        fft_real[row2 * (sel_width + padding) + col2] = fft_real[(sel_height - row2) * (sel_width + padding) + col2];
      }
      fft_real[col2 + 1] = 0;
      if (sel_height % 2 == 0)
        fft_real[sel_height / 2 * (sel_width + padding) + col2 + 1] = 0;
    }
    // do not unboost (0, 0), just offset it
    row = sel_height / 2;
    col = sel_width / 2;
    v = get_double128(row, col, src_pixels[(row * sel_width + col) * src_bpp + cur_bpp]);
    fft_real[0] = v + 128.0;

    gimp_progress_update((double)++progress / max_progress);
    fftw_execute(p);
    gimp_progress_update((double)++progress / max_progress);
    for (col = 0; col < sel_width; col++)
    {
      for (row = 0; row < sel_height; row++)
      {
        v = fft_real[row * (sel_width + padding) + col];
        dst_pixels[(row * sel_width + col) * dst_bpp + cur_bpp] = get_guchar(col, row, v);
      }
    }
    gimp_progress_update((double)++progress / max_progress);
  }

  fftw_destroy_plan(p);
  g_free(fft_real);
}