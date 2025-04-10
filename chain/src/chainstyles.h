// # MIT License
// # Copyright (c) 2025 Stephen J. Parker
// # SPDX-License-Identifier: MIT
// # See LICENSE file for full license text.
//

#pragma once
#include <iostream>
#include <QPalette>
#include <QtNodes/StyleCollection>

using QtNodes::GraphicsViewStyle;
using QtNodes::NodeStyle;
using QtNodes::ConnectionStyle;


inline QPalette
darkPalette()
{
    QColor darkGray (53, 53, 53);
    QColor gray (128, 128, 128);
    QColor black (25, 25, 25);
    QColor blue (54, 132, 210);
    QColor light (220, 220, 220);
    QColor white (240, 240, 240);

    QPalette darkPalette;

    darkPalette.setColor (QPalette::Window, darkGray);
    darkPalette.setColor (QPalette::WindowText, light);
    darkPalette.setColor (QPalette::Base, black);
    darkPalette.setColor (QPalette::AlternateBase, darkGray);
    darkPalette.setColor (QPalette::ToolTipBase, blue);
    darkPalette.setColor (QPalette::ToolTipText, light);
    darkPalette.setColor (QPalette::Text, light);
    darkPalette.setColor (QPalette::Button, darkGray);
    darkPalette.setColor (QPalette::ButtonText, light);
    darkPalette.setColor (QPalette::BrightText, white);
    darkPalette.setColor (QPalette::Link, blue);
    darkPalette.setColor (QPalette::Highlight, blue);
    darkPalette.setColor (QPalette::HighlightedText, Qt::black);

    darkPalette.setColor (QPalette::Active, QPalette::Button, gray.darker());
    darkPalette.setColor (QPalette::Disabled, QPalette::ButtonText, gray);
    darkPalette.setColor (QPalette::Disabled, QPalette::WindowText, gray);
    darkPalette.setColor (QPalette::Disabled, QPalette::Text, gray);
    darkPalette.setColor (QPalette::Disabled, QPalette::Light, darkGray);

    return darkPalette;
} // darkPalette


inline void
setNodeStyle()
{
    GraphicsViewStyle::setStyle (
        R"(
        {
        "GraphicsViewStyle": {
            "BackgroundColor": [40, 40, 40],
            "FineGridColor": [50, 50, 50],
            "CoarseGridColor": [25, 25, 25]
            }
        }
        )"
    );

    NodeStyle::setNodeStyle (
        R"(
        {
        "NodeStyle": {
            "NormalBoundaryColor": "black",
            "SelectedBoundaryColor": [229, 165, 0],
            "GradientColor0": [100, 100, 100],
            "GradientColor1": [80, 80, 80],
            "GradientColor2": [50, 50, 50],
            "GradientColor3": [45, 45, 45],
            "ShadowColor": [10, 10, 10, 127],
            "FontColor" : [200, 200, 200],
            "FontColorFaded" : "gray",
            "ConnectionPointColor": [200, 200, 200],
            "FilledConnectionPointColor": [229, 165, 0],
            "ErrorColor": "red",
            "WarningColor": [128, 128, 0],
            "PenWidth": 1.0,
            "HoveredPenWidth": 1.5,
            "ConnectionPointDiameter": 9.0,
            "Opacity": 1.0
            }
        }
        )"
    );

    ConnectionStyle::setConnectionStyle (
        R"(
        {
        "ConnectionStyle": {
            "ConstructionColor": "gray",
            "SelectedHaloColor": [184, 128, 11],
            "SelectedColor": [234, 156, 11],
            "NormalColor": [50, 120, 85],
            "HoveredColor": [55, 200, 140],
            "LineWidth": 3.0,
            "ConstructionLineWidth": 2.0,
            "PointDiameter": 10.0,
            "UseDataDefinedColors": false
            }
        }
        )"
    );
}


const std::string chainScrollBarQss = R"(
    QScrollBar:vertical{
    background:palette(base);
    border-top-right-radius:2px;
    border-bottom-right-radius:2px;
    width:16px;
    margin:0px;
    }
    QScrollBar::handle:vertical{
    background-color:palette(alternate-base);
    border-radius:2px;
    min-height:20px;
    margin:2px 4px 2px 4px;
    }
    QScrollBar::handle:vertical:hover{
    background-color:palette(highlight);
    }
    QScrollBar::add-line:vertical{
    background:none;
    height:0px;
    subcontrol-position:right;
    subcontrol-origin:margin;
    }
    QScrollBar::sub-line:vertical{
    background:none;
    height:0px;
    subcontrol-position:left;
    subcontrol-origin:margin;
    }
    QScrollBar:horizontal{
    background:palette(base);
    height:16px;
    margin:0px;
    }
    QScrollBar::handle:horizontal{
    background-color:palette(alternate-base);
    border-radius:2px;
    min-width:20px;
    margin:4px 2px 4px 2px;
    }
    QScrollBar::handle:horizontal:hover{
    background-color:palette(highlight);
    }
    QScrollBar::add-line:horizontal{
    background:none;
    width:0px;
    subcontrol-position:bottom;
    subcontrol-origin:margin;
    }
    QScrollBar::sub-line:horizontal{
    background:none;
    width:0px;
    subcontrol-position:top;
    subcontrol-origin:margin;
    }
    QSlider::handle:horizontal{
    border-radius:4px;
    border:1px solid rgba(25,25,25,255);
    background-color:palette(alternate-base);
    min-height:20px;
    margin:0 -4px;
    }
    QSlider::handle:horizontal:hover{
    background:palette(highlight);
    }
    QSlider::add-page:horizontal{
    background:palette(base);
    }
    QSlider::sub-page:horizontal{
    background:palette(highlight);
    }
    QSlider::sub-page:horizontal:disabled{
    background:rgb(80,80,80);
    }
)";
