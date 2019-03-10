/*!  \file     gui_carousel.h
*    \brief    GUI carousel rendering functions
*    Created:  17/11/2018
*    Author:   Mathieu Stephan
*/
#include <stdio.h>

#ifndef GUI_CAROUSEL_H_
#define GUI_CAROUSEL_H_

/* Defines */

// Bitmap IDs
#define BITMAP_BATTERY_0PCT_ID      662
#define BITMAP_TRAY_BT_OFF          668
#define BITMAP_TRAY_BT_CONNECTED_ID 668
#define BITMAP_TRAY_BT_ON           670

// Big, medium, small icon X/Y
#define CAROUSEL_BIG_EDGE           48
#define CAROUSEL_MID_EDGE           32
#define CAROUSEL_SMALL_EDGE         24
// Edge increment for animation
#define CAROUSEL_Y_ANIM_STEP        2
// Number of bitmap for given icon
#define CAROUSEL_NB_SCALED_ICONS    9
// Y on each all icons are aligned
#define CAROUSEL_Y_ALIGN            24
// Number of animation steps
#define CAROUSEL_NB_ANIM_STEPS      (CAROUSEL_NB_SCALED_ICONS/2)
// Available space for a given number of icons
#define CAROUSEL_AV_SPACE(x)        (GUI_DISPLAY_WIDTH - (CAROUSEL_BIG_EDGE + 2*CAROUSEL_MID_EDGE + ((x)-3)*CAROUSEL_SMALL_EDGE))
// Spacing between icons
#define CAROUSEL_IS_SM(x)           (CAROUSEL_AV_SPACE((x)) / (x))
// Spacing on the left of carousel
#define CAROUSEL_LS_SM(x)           ((CAROUSEL_IS_SM((x)) / 2) + ((CAROUSEL_AV_SPACE((x)) - CAROUSEL_IS_SM((x))*(x)) / 2))
// X offset step for carousel animation
#define CAROUSEL_X_STEP_ANIM(x)     (((CAROUSEL_IS_SM(x))+CAROUSEL_MID_EDGE)/CAROUSEL_NB_ANIM_STEPS - 2)

/* Prototypes */
void gui_carousel_render_animation(uint16_t nb_elements, const uint16_t* pic_ids, const uint16_t* text_ids, uint16_t selected_id, BOOL left_anim);
void gui_carousel_render(uint16_t nb_elements, const uint16_t* pic_ids, const uint16_t* text_ids, uint16_t selected_id, int16_t anim_step);


#endif /* GUI_CAROUSEL_H_ */