extern bool ctrl_sdof_create_buffers(void);
extern void ctrl_sdof_destroy_buffers(void);
extern void ctrl_sdof_recreate_buffers(int w, int h);

// To do shallow depth of field rendering, we need 5 offscreen renderbuffers.
// The render stages are: normal render, coc, subsample, blurh, blurv.
// In this function we create those buffers, and create shaders for the stages.
extern bool ctrl_sdof_create( int backingW, int backingH, int sf );

// Clean up the shader programs and the render buffers related to shallow depth of field post effect.
extern void ctrl_sdof_destroy( void );

// Render all offscreen passes, required for shallow depth of field.
extern void ctrl_sdof_draw_offscreen( void );

// Do final render combining original and blurred buffers, typically into on-screen buffer.
extern void ctrl_sdof_draw_final( void );

extern float ctrl_sdof_focal_dist;
extern float ctrl_sdof_fgnd_blur;
extern float ctrl_sdof_bgnd_blur;
