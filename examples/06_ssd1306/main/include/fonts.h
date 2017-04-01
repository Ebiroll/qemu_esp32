/**
  ******************************************************************************
  * @file    fonts.h
  * @author  Baoshi <mail(at)ba0sh1(dot)com>
  * @version 0.8
  * @date    Jan 3, 2014
  * @brief   Font description for OLED display, based on
  *          <a href="http://www.eran.io/the-dot-factory-an-lcd-font-and-image-generator/">
  *          "TheDotFactory"</a> by Eran "Pavius" Duchan
  *
  ******************************************************************************
  * @copyright
  *
  * Copyright (c) 2015, Baoshi Zhu. All rights reserved.
  * Use of this source code is governed by a BSD-style license that can be
  * found in the LICENSE.txt file.
  *
  * THIS SOFTWARE IS PROVIDED 'AS-IS', WITHOUT ANY EXPRESS OR IMPLIED
  * WARRANTY.  IN NO EVENT WILL THE AUTHOR(S) BE HELD LIABLE FOR ANY DAMAGES
  * ARISING FROM THE USE OF THIS SOFTWARE.
  *
  */

#ifndef FONTS_H
#define FONTS_H
#include "stdint.h"

//! @brief Character descriptor
typedef struct _font_char_desc
{
    uint8_t width;      //!< Character width in pixel
    uint16_t offset;    //!< Offset of this character in bitmap
} font_char_desc_t;


//! @brief Font information
typedef struct _font_info
{
    uint8_t height;         //!< Character height in pixel, all characters have same height
    uint8_t c;              //!< Simulation of "C" width in TrueType term, the space between adjacent characters
    char char_start;        //!< First character
    char char_end;          //!< Last character
    const font_char_desc_t* char_descriptors; //! descriptor for each character
    const uint8_t *bitmap;  //!< Character bitmap
} font_info_t;


#define NUM_FONTS 2    //!< Number of built-in fonts

extern const font_info_t * fonts[NUM_FONTS];  //!< Built-in fonts


#endif /* FONTS */
