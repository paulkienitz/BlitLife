/* grow Life generations with the blitter -- my very first program making
direct hardware use of the ol' blit-thing.  COMPILE WITH FAR DATA, with
initialized data in chip ram if you want to use the pattern thing. */

#include <exec/exec.h>
#include <hardware/custom.h>
#include <hardware/blit.h>
#include <intuition/intuition.h>
#include <graphics/gfxbase.h>
#include <graphics/gfxmacros.h>
#include <workbench/startup.h>
#include <Paul.h>


extern struct Custom custom;


#define WIDE 320
#define HIGH 200
#define TITLEOFF 12

#define ROWB (((WIDE + 32 + 15) >> 3) & 0xFFFE)


struct BitMap bap = {
    ROWB, HIGH, 0, 1, 0, { null }
};
/* this bitmap differs from a normal screen bitmap in that there is a
"margin" of one row at top and bottom, and one word along each side, filled
with zeroes.  This may not be necessary but I'm putting it in just so I have
fewer worries.  It is one simple bitplane. */


ubyte tit[] = "SPACE=grow ESC=quit DEL=clear G=go";
ubyte jenny[40];


struct TextAttr top8 = { "topaz.font", 8, 0, 0 };

struct NewScreen news = {
    0, 0, WIDE, HIGH + TITLEOFF, 1, 0, 1, 0L, CUSTOMSCREEN | CUSTOMBITMAP,
    &top8, &tit[0], null, &bap
};


struct NewWindow neww = {
    0, TITLEOFF, WIDE, HIGH, 0, 0, MOUSEBUTTONS | MOUSEMOVE | VANILLAKEY,
    BORDERLESS | BACKDROP | REPORTMOUSE | NOCAREREFRESH, null, null, null,
    null, null, 0, 0, 0, 0, CUSTOMSCREEN
};

struct Screen *ss;


PLANEPTR b1, b2, b4, bt, bT;            /* bitmaps, with margins */

#define ROFF ROWB + 2

#define MS RASSIZE(WIDE + 32, HIGH + 2)
#define MC MEMF_CHIP

struct PseudoMemList {
    struct Node noad;
    ushort numentries;
    ulong r1, s1, r2, s2, r3, s3, r4, s4, r5, s5, r6, s6;
} pms = {
    { 0 }, 6, MC, MS, MC, MS, MC, MS, MC, MS, MC, MS,
    MC | MEMF_CLEAR, MS + RASSIZE(WIDE + 32, TITLEOFF)
};


ushort pattern[3] = { 0xDB6D, 0xB6DB, 0x6DB6 };

long gen = 0;


adr IntuitionBase;
struct GfxBase *GfxBase;



long _main(long alen)
{
    struct Window *ww;
    struct MemList *temps;
    struct WBStartup *wbs = null;
    long r = 20;
    void Eventz(struct Window *ww), wipe(void);

    if (!alen) {
        struct MsgPort *mmp = &ThisProcess()->pr_MsgPort;
        WaitPort(mmp);
        wbs = (adr) GetMsg(mmp);
    }
    if (!(GfxBase = (adr) OpenLibrary("graphics.library", 0L))) goto fail0;
    if (!(IntuitionBase = OpenLibrary("intuition.library", 0L))) goto fail1;
    if ((temps = AllocEntry((struct MemList *) &pms)) < 0) goto fail2;
    b1 = (adr) ((PLANEPTR) temps->ml_ME[0].me_Addr + ROFF);
    b2 = (adr) ((PLANEPTR) temps->ml_ME[1].me_Addr + ROFF);
    b4 = (adr) ((PLANEPTR) temps->ml_ME[2].me_Addr + ROFF);
    bt = (adr) ((PLANEPTR) temps->ml_ME[3].me_Addr + ROFF);
    bT = (adr) ((PLANEPTR) temps->ml_ME[4].me_Addr + ROFF);
    bap.Planes[0] = temps->ml_ME[5].me_Addr;
    news.LeftEdge = (GfxBase->NormalDisplayColumns - 640) >> 2;
    if (!(ss = OpenScreen(&news))) goto fail3;
    SetRGB4(&ss->ViewPort, 0L, 3L, 5L, 6L);
    SetRGB4(&ss->ViewPort, 1L, 15L, 15L, 8L);
    wipe();
    neww.Screen = ss;
    if (!(ww = OpenWindow(&neww))) goto fail4;
    r = 0;
    Eventz(ww);
    CloseWindow(ww);
fail4:
    CloseScreen(ss);
fail3:
    FreeEntry(temps);
fail2:
    CloseLibrary(IntuitionBase);
fail1:
    CloseLibrary((adr) GfxBase);
fail0:
    if (wbs) {
        Forbid();
        ReplyMsg((adr) wbs);
    }
    return r;
}



void wipe(void)
{
    SetAPen(&ss->RastPort, 0L);
    RectFill(&ss->RastPort, 0L, TITLEOFF - 1L, (long) WIDE, TITLEOFF - 1L);
}



void Eventz(struct Window *ww)
{
    struct MsgPort *wip = ww->UserPort;
    struct RastPort *rp = ww->RPort;
    struct IntuiMessage im, *tim;
    static bool liney = false, whiteline = false;
    static short sx, sy, ex, ey;
    void Grow(struct Window *ww);
    short i, j;
    long k;

    SetAPen(rp, 1L);
    for (;;) {
        WaitPort(wip);
        while (tim = (adr) GetMsg(wip)) {
            im = *tim;
            ReplyMsg((adr) tim);
            switch (im.Class) {
                case VANILLAKEY:
                    switch (im.Code) {
                        case  27:                       /* escape */
                            return;
                        case ' ':                       /* space */
                            Grow(ww);
                            sprintf(jenny, "Generation %ld", gen);
                            SetWindowTitles(ww, null, jenny);
                            wipe();
                            break;
                        case 'g': case 'G':
                            ReportMouse1(ww, 0L);
                            SetWindowTitles(ww, null, (ustr)
                                " .... floob ... dloit ... RoR ... ");
                            wipe();
                            while (!(SetSignal(0L, 0L) & bit(wip->mp_SigBit))) {
                                Grow(ww);
                            }
                            ReportMouse1(ww, 1L);
                            sprintf(jenny, "Generation %ld", gen);
                            SetWindowTitles(ww, null, jenny);
                            wipe();
                            break;
                        case 'p': case 'P':
                            SetAPen(rp, 1L);
                            SetBPen(rp, 0L);
                            for (i = 0; ; i += 3)
                                for (j = 0; j < 3; j++) {
                                    if (i + j >= WIDE >> 4)
                                        goto did;
                                    SetAfPt(rp, &pattern[j], 0L);
                                    SetDrMd(rp, (long) JAM2);
                                    k = (i + j) << 4;
                                    RectFill(rp, k, 0L, k + 15, (long) HIGH);
                                }
                          did:
                            SetAPen(rp, 0L);
                            for (k = 2; k < HIGH; k += 3)
                                RectFill(rp, 0, k, WIDE - 1L, k);
                            gen = 0;
                            SetWindowTitles(ww, null, tit);
                            wipe();
                            break;
                        case 127:                       /* del */
                            Move(rp, 0L, 0L);
                            ClearScreen(rp);
                            gen = 0;
                            SetWindowTitles(ww, null, tit);
                            wipe();
                    }
                    break;
                case MOUSEBUTTONS:
                    if (im.Code == SELECTDOWN) {
                        if (im.Qualifier & (IEQUALIFIER_LSHIFT
                                            | IEQUALIFIER_RSHIFT)) {
                            ex = sx = im.MouseX;
                            ey = sy = im.MouseY;
                            whiteline = !ReadPixel(rp, (long) sx, (long) sy);
                            liney = true;
                        } else {
                            SetAPen(rp, (long) !ReadPixel(rp, (long) im.MouseX,
                                                          (long) im.MouseY));
                            WritePixel(rp, (long) im.MouseX, (long) im.MouseY);
                        }
                    } else if (im.Code == SELECTUP && liney &&
                                        (sx != im.MouseX || sy != im.MouseY)) {
                        SetAPen(rp, (long) whiteline);
                        SetDrMd(rp, (long) JAM2);
                        Move(rp, (long) sx, (long) sy);
                        Draw(rp, (long) im.MouseX, (long) im.MouseY);
                        liney = false;
                    } else liney = false;
                    break;
                case MOUSEMOVE:
                    if (!liney) {
                        if (im.Qualifier & IEQUALIFIER_LEFTBUTTON)
                            WritePixel(rp, (long) im.MouseX, (long) im.MouseY);
                    } else {
                        SetDrMd(rp, (long) COMPLEMENT);
                        if (sx != ex || sy != ey) {
                            Move(rp, (long) sx, (long) sy);
                            Draw(rp, (long) ex, (long) ey);
                        }
                        ex = im.MouseX;
                        ey = im.MouseY;
                        if (sx != ex || sy != ey) {
                            Move(rp, (long) sx, (long) sy);
                            Draw(rp, (long) ex, (long) ey);
                        }
                    }
                    break;
            }
        }
    }
}



#define ODDNUM  ABC | ANBNC | NABNC | NANBC
#define ANYTWO  ABC | ABNC | ANBC | NABC
#define AB_X_C  ABNC | ANBC | NABC | NANBC
#define ANB_X_C ABC | ANBNC | NABC | NANBC
#define ANYONE  ABC | ABNC | ANBC | ANBNC | NABC | NABNC | NANBC
#define AXB_ORC ABC | ANBC | ANBNC | NABC | NABNC | NANBC


void Grow(struct Window *ww)
{
    PLANEPTR mm = ww->RPort->BitMap->Planes[0] + ROWB * TITLEOFF;
    void Blit(PLANEPTR d, PLANEPTR s1, short s1s, PLANEPTR s2, ushort s2s,
                PLANEPTR s3, short mint);
    /* NOTE s1s (channel A shift) is signed, s2s (B shift) is unsigned */

    Blit(b1, mm - ROWB, -1, mm - ROWB, 1, mm - ROWB, ODDNUM);
    Blit(b2, mm - ROWB, -1, mm - ROWB, 1, mm - ROWB, ANYTWO);
    Blit(b4, mm, -1, b1, 0, b2, ABC);
    Blit(b2, mm, -1, b1, 0, b2, AB_X_C);
    Blit(b1, mm, -1, mm, 1, b1, ODDNUM);
    Blit(bt, mm, 1, b1, 0, b2, ANBC);
    Blit(b2, mm, 1, b1, 0, b2, ANB_X_C);
    Blit(bT, mm + ROWB, -1, b1, 0, b2, ABC);
    Blit(b2, mm + ROWB, -1, b1, 0, b2, AB_X_C);
    Blit(b4, bt, 0, bT, 0, b4, ANYONE);
    Blit(b1, mm + ROWB, -1, mm + ROWB, 0, b1, ODDNUM);
    Blit(bt, mm + ROWB, 0, b1, 0, b2, ANBC);
    Blit(b2, mm + ROWB, 0, b1, 0, b2, ANB_X_C);
    Blit(bT, mm + ROWB, 1, b1, 0, b2, ABC);
    Blit(b2, mm + ROWB, 1, b1, 0, b2, AB_X_C);
    Blit(b4, bt, 0, bT, 0, b4, ANYONE);
    Blit(bt, mm + ROWB, 1, b1, 0, mm, AXB_ORC);
    WaitTOF();                                  /* cosmetic */
    Blit(mm, bt, 0, b2, 0, b4, ABNC);
    /* eighteen bronze blits */
    gen++;
}



void Blit(PLANEPTR d, PLANEPTR s1, short s1s, PLANEPTR s2, ushort s2s,
                PLANEPTR s3, short mint)
{
    ushort ash, bsh = s2s, dma = (s3 ? 0xF00 : 0xD00);
    ushort wide = (WIDE + 15) >> 4;
    OwnBlitter();
    WaitBlit();
    if (s1s < 0) {
        ash = s1s + 16;
        s2 -= 2;
        s3 -= 2;
        d -= 2;
        custom.bltamod = custom.bltbmod = custom.bltcmod = custom.bltdmod = 2;
        wide++;
    } else {
        ash = s1s;
        custom.bltamod = custom.bltbmod = custom.bltcmod = custom.bltdmod = 4;
    }
    custom.bltapt = s1;
    custom.bltbpt = s2;
    custom.bltcpt = s3;
    custom.bltdpt = d;
    custom.bltcon0 = (ash << 12) | dma | mint;
    custom.bltcon1 = bsh << 12;                         /* zero all flags */
    custom.bltalwm = 0xFFFF << ash;
    custom.bltafwm = 0xFFFF;
    custom.bltsize = (HIGH << HSIZEBITS) + wide;        /* Go! */
    DisownBlitter();
}
