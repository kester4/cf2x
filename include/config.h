#pragma once

#define  MAX_EQUATIONS (200)
#define           SSAA (2)        // set this to 1 if you get framebuffer warnings
#define  INPUTLEN_INIT (64)

// window parameters
#define  INITIAL_WIDTH (1280)
#define INITIAL_HEIGHT (900)

// ratios
#define   INPUTBOX_REL (5)        // plots canvas : input column (width)
#define INPUT_HEAD_REL (0.05f)    // input column head : full input column (height)
#define    INPUT_H_REL (0.067f)   // single input field : full inputs column (height)

// sizes
#define   INPUTB_THICK (2) 
#define INPUTB_PADDING (15.0f)

// plotting canvas parameters
#define   GRID_SPACING (140)
#define   MINORL_COUNT (3)
#define   MINORL_THICK (1)
#define     AXIS_THICK (3)
#define    GRAPH_THICK (3)
#define   MAJORL_THICK (2)
#define  INITIAL_SCALE (30.0)
#define    ZOOM_FACTOR (1.1)
#define  PERIODIC_FUNC (8.0f)     // see render_plot()

// labels formatting
#define   MAX_SHORTVAL (1e5)
#define      FONT_SIZE (20.0f)

// adaptive function sampling
#define   MAX_RECDEPTH (55)    
#define   TOLERANCE_PX (0.35f)

#define CLAMP(val, min, max) \
	((val) < (min) ? (min) : (val) > (max) ? (max) : (val))

// color pallete
#define     BGND_LIGHT ((Color){ 245, 245, 245, 255 })
#define   MJGRID_LIGHT ((Color){ 180, 180, 180, 255 })
#define   MNGRID_LIGHT ((Color){ 200, 200, 200, 255 })
#define     AXIS_LIGHT ((Color){ 80, 80, 80, 255 })
#define     TEXT_LIGHT ((Color){ 30, 30, 30, 255 })
#define INPUTBOX_LIGHT ((Color){ 250, 250, 250, 255 })
#define  INPUTUP_LIGHT ((Color){ 215, 215, 215, 255})
#define    ERASE_LIGHT (INPUTBOX_LIGHT)

#define      BGND_DARK ((Color){ 40, 40, 40, 255 })
#define    MJGRID_DARK ((Color){ 82, 82, 82, 255 })
#define    MNGRID_DARK ((Color){ 69, 69, 69, 255 })
#define      AXIS_DARK ((Color){ 184, 184, 184, 255 })
#define      TEXT_DARK ((Color){ 245, 245, 245, 255 })
#define  INPUTBOX_DARK ((Color){ 43, 43, 43, 255 })
#define   INPUTUP_DARK ((Color){ 65, 65, 65, 255 })
#define     ERASE_DARK (INPUTBOX_DARK)