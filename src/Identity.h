#pragma once

// Maskinens identitet.
//
// Alla maskiner i MASKINHALLEN delar samma kod. Det som skiljer dem är namnet
// och identiteten i DAW:en, och den sätts av CMake per mål (se BETONG_MACHINES
// i CMakeLists.txt). Här ligger bara standardvärdena, som används om filen
// skulle kompileras utanför CMake.
//
// Viktigt: håll värdena i ASCII. De hamnar i DAW:ens pluginlista, i bundle-namn
// och i sökvägar, och ska sökas på utan att krångla.

#ifndef BETONG_PRODUCT_NAME
 #define BETONG_PRODUCT_NAME "BETONG"                // namnet i DAW:en och i loggan
#endif

#ifndef BETONG_ABOUT_LINE
 #define BETONG_ABOUT_LINE "techno-sampler"          // raden under namnet på om-sidan
#endif

#ifndef BETONG_TAGLINE
 #define BETONG_TAGLINE "TECHNO SAMPLER"             // raden under loggan i huvudvyn
#endif

#ifndef BETONG_DEFAULT_PATTERN
 #define BETONG_DEFAULT_PATTERN "Grund 130"          // mönstret maskinen öppnar med
#endif
