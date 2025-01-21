
#define THRESHOLDSREVOX30DB
//#define THRESHOLDS30DB
//#define THRESHOLDS40DB

#define INZERODB     1020

#ifdef THRESHOLDSREVOX30DB
float thresholds[NUMLEDS] = {  // manually set steps to compensate for the unlinear rectification in the Revox and non-balanced-input measurement with this board.
0.0168*INZERODB,   // -30dB
0.0196*INZERODB,   // -29dB
0.0234*INZERODB,   // -28dB
0.0271*INZERODB,   // -27dB
0.0318*INZERODB,   // -26dB
0.0365*INZERODB,   // -25dB
0.0430*INZERODB,   // -24dB
0.0496*INZERODB,   // -23dB
0.0571*INZERODB,   // -22dB
0.0664*INZERODB,   // -21dB
0.0767*INZERODB,   // -20dB
0.0889*INZERODB,   // -19dB
0.1020*INZERODB,   // -18dB
0.1169*INZERODB,   // -17dB
0.1347*INZERODB,   // -16dB
0.1534*INZERODB,   // -15dB
0.1759*INZERODB,   // -14dB
0.2002*INZERODB,   // -13dB
0.2283*INZERODB,   // -12dB
0.2591*INZERODB,   // -11dB
0.2947*INZERODB,   // -10dB
0.3340*INZERODB,   // -9dB
0.3798*INZERODB,   // -8dB
0.4294*INZERODB,   // -7dB
0.4846*INZERODB,   // -6dB
0.5454*INZERODB,   // -5dB
0.6155*INZERODB,   // -4dB
0.6950*INZERODB,   // -3dB
0.7858*INZERODB,   // -2dB
0.8859*INZERODB,   // -1dB
1.0000*INZERODB,   // 0dB
1.1272*INZERODB,   // +1dB
1.2694*INZERODB,   // +2dB
1.4284*INZERODB,   // +3dB
1.6034*INZERODB,   // +4dB
1.7961*INZERODB    // +5dB
};
#endif

#ifdef THRESHOLDS40DB
float thresholds[NUMLEDS] = {    // true dB scale, can be used instead of the recalibrated above if you connect the inputs to somewhere in the clean audio chain and NOT the Revox's rectifier
0.0100*INZERODB,    // -40dB
0.0126*INZERODB,    // -38dB
0.0158*INZERODB,    // -36dB
0.0200*INZERODB,    // -34dB
0.0251*INZERODB,    // -32dB
0.0316*INZERODB,    // -30dB
0.0398*INZERODB,    // -28dB
0.0501*INZERODB,    // -26dB
0.0631*INZERODB,    // -24dB
0.0794*INZERODB,    // -22dB
0.1000*INZERODB,    // -20dB
0.1122*INZERODB,    // -19dB
0.1259*INZERODB,    // -18dB
0.1413*INZERODB,    // -17dB
0.1585*INZERODB,    // -16dB
0.1778*INZERODB,    // -15dB
0.1995*INZERODB,    // -14dB
0.2239*INZERODB,    // -13dB
0.2512*INZERODB,    // -12dB
0.2818*INZERODB,    // -11dB
0.3162*INZERODB,    // -10dB
0.3548*INZERODB,    // -9dB
0.3981*INZERODB,    // -8dB
0.4467*INZERODB,    // -7dB
0.5012*INZERODB,    // -6dB
0.5623*INZERODB,    // -5dB
0.6310*INZERODB,    // -4dB
0.7079*INZERODB,    // -3dB
0.7943*INZERODB,    // -2dB
0.8913*INZERODB,    // -1dB
1.0000*INZERODB,    // 0dB
1.1220*INZERODB,    // +1dB
1.2589*INZERODB,    // +2dB
1.4126*INZERODB,    // +3dB
1.5849*INZERODB,    // +4dB
1.7783*INZERODB     // +5dB
};
#endif

#ifdef THRESHOLDS30DB
float thresholds[NUMLEDS] = {    // true dB scale, can be used instead of the recalibrated above if you connect the inputs to somewhere in the clean audio chain and NOT the Revox's rectifier
0.0316*INZERODB,    // -30dB
0.0355*INZERODB,    // -29dB
0.0398*INZERODB,    // -28dB
0.0447*INZERODB,    // -27dB
0.0501*INZERODB,    // -26dB
0.0526*INZERODB,    // -25dB
0.0631*INZERODB,    // -24dB
0.0708*INZERODB,    // -23dB
0.0794*INZERODB,    // -22dB
0.0891*INZERODB,    // -21dB
0.1000*INZERODB,    // -20dB
0.1122*INZERODB,    // -19dB
0.1259*INZERODB,    // -18dB
0.1413*INZERODB,    // -17dB
0.1585*INZERODB,    // -16dB
0.1778*INZERODB,    // -15dB
0.1995*INZERODB,    // -14dB
0.2239*INZERODB,    // -13dB
0.2512*INZERODB,    // -12dB
0.2818*INZERODB,    // -11dB
0.3162*INZERODB,    // -10dB
0.3548*INZERODB,    // -9dB
0.3981*INZERODB,    // -8dB
0.4467*INZERODB,    // -7dB
0.5012*INZERODB,    // -6dB
0.5623*INZERODB,    // -5dB
0.6310*INZERODB,    // -4dB
0.7079*INZERODB,    // -3dB
0.7943*INZERODB,    // -2dB
0.8913*INZERODB,    // -1dB
1.0000*INZERODB,    // 0dB
1.1220*INZERODB,    // +1dB
1.2589*INZERODB,    // +2dB
1.4126*INZERODB,    // +3dB
1.5849*INZERODB,    // +4dB
1.7783*INZERODB     // +5dB
};
#endif
