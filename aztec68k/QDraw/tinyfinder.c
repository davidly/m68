/*
 * TinyFinder.c - 21-Feb-85
 *
 * This program is an alternative Finder that is very fast and small,
 * and limited in its abilities compared to the real Finder.  It is
 * written in Aztec 68000 C (version 1.06D).
 *
 * copyright (c) 1985 by:
 *   Kent Kersten
 *   Harrison Systems
 *   437 Atlas Drive
 *   Nashville, TN  37211
 *   (615) 834-1366
 *   CompuServe CIS [72277,2726]
 *
 * To compile & link this file:
 *       cc TinyFinder.c
 *       ln -m TinyFinder.o sys:lib/sacroot.o -lc
 */
 
#include  <quickdraw.h>
#include  <window.h>
#include  <event.h>
#include  <menu.h>
#include  <desk.h>
#include  <dialog.h>
#include  <resource.h>
#include  <segment.h>
#include  <packages.h>
#include  <pb.h>
#include  <syserr.h>
#include  <inits.h>
 
#define   nil                   (long) 0
#define   NUM_MENUS             4
 
#define   AppleMenu             256
#define   FileMenu              257
#define   EditMenu              258
#define   SpecialMenu           259
 
#define   ABOUT_DLOG            256     /* resource id # of our 'About...' box */
#define   GENERAL_ERROR_ALRT    500
 
MenuHandle TFMenu[NUM_MENUS];
Point      whereOpen    = { 80, 80 };           /* where to put the SF dialog */
 
Boolean  DoCommand(), ProcessEvents();
 
 
main()
{
     InitGraf(&thePort);
     InitFonts();
     InitWindows();
     InitMenus();
     InitDialogs(nil);
     TEInit();
 
     FlushEvents(everyEvent, 0);        /* flush ALL events */
     FormMenuBar();
 
     CheckForFinder();
 
     InitCursor();
     LaunchAppl();
 
     while(ProcessEvents())
          ;
}
 
 
/*
 * Get events and process them.  Return FALSE to exit.
 */
 
Boolean ProcessEvents()
{
     char         theChar;
     Boolean      NoQuitFlag = TRUE;
     WindowPtr    whichWindow;
     EventRecord  TFEvent;
     GrafPtr      savePort;
 
     HiliteMenu(0);                     /* turn off any highlighting in menu bar */
     SystemTask();
 
     GetNextEvent(everyEvent, &TFEvent);
     switch(TFEvent.what) {
          case mouseDown:
               switch(FindWindow(pass(TFEvent.where), &whichWindow)) {  /* major Aztec C68K hack-ack (pass) */
                    case inMenuBar:
                         NoQuitFlag = DoCommand(MenuSelect(pass(TFEvent.where)));       /* Aztec hack */
                         break;
                    case inSysWindow:
                         SystemClick(&TFEvent,whichWindow);
                         break;
               }
               break;
          case keyDown:
          case autoKey:
               theChar = TFEvent.message % 256;
               if((TFEvent.modifiers & cmdKey) != 0)            /* command key pressed */
                    NoQuitFlag = DoCommand(MenuKey(theChar));
               break;
          case updateEvt:
               GetPort(&savePort);
               SetPort(TFEvent.message);
               BeginUpdate(TFEvent.message);
               EndUpdate(TFEvent.message);
               SetPort(savePort);
               break;
     }
 
     return NoQuitFlag;
}
 
 
/*
 * Decipher menu commands here.  Return FALSE to quit.
 */
 
Boolean DoCommand(command)
long command;
{
     char     name[256];
     short    MenuNum, ItemNum;
     Boolean  NoQuitFlag = TRUE;
 
     InitCursor();
     MenuNum = HiWord(command);
     ItemNum = LoWord(command);
 
     switch(MenuNum) {
          case AppleMenu:
               if(ItemNum == 1)
                    AboutTinyFinder();          /* do our "About..." box */
               else {
                    GetItem(TFMenu[0], ItemNum, name);
                    OpenDeskAcc(name);
               }
               break;
          case FileMenu:
               switch(ItemNum) {
                    case 1:
                         LaunchAppl();
                         break;
                    case 2:
                         GoFinder();
                         break;
                    case 4:
                         NoQuitFlag = FALSE;
                         break;
               }
               break;
          case EditMenu:
               switch(ItemNum) {
                    case 1:
                         SystemEdit(undoCmd);
                         break;
                    case 3:
                         SystemEdit(cutCmd);
                         break;
                    case 4:
                         SystemEdit(copyCmd);
                         break;
                    case 5:
                         SystemEdit(pasteCmd);
                         break;
                    case 6:
                         SystemEdit(clearCmd);
                         break;
               }
               break;
          case SpecialMenu:
               switch(ItemNum) {
                    case 1:
                         ShutDown();
                         break;
               }
     }
 
     return NoQuitFlag;
}
 
 
/*
 * This pops up a dialog for our 'About...' box.
 */
 
AboutTinyFinder()
{
     int         type, ItemHit;
     Handle      hdl;
     DialogPtr   DPtr;
     Rect        tBox;
 
     InitCursor();
 
     DPtr = GetNewDialog(ABOUT_DLOG, nil, (long) -1);
     SetPort(DPtr);
 
     GetDItem(DPtr, 2, &type, &hdl, &tBox);             /* icon */
     PlotIcon(&tBox, GetIcon(256));
 
     PenPat(&gray);
     GetDItem(DPtr, 6, &type, &hdl, &tBox);             /* dividing line */
     MoveTo(tBox.left, tBox.top);
     LineTo(tBox.right, tBox.top);
 
     GetDItem(DPtr, 7, &type, &hdl, &tBox);             /* dividing line */
     MoveTo(tBox.left, tBox.top);
     LineTo(tBox.right, tBox.top);
     PenPat(&black);
 
     while(1) {
          ModalDialog(nil, &ItemHit);
          if(ItemHit == 1) {
               CloseDialog(DPtr);                       /* 'OK' */
               break;
          }
     }
}
 
 
/*
 * Check to see if the file named 'Finder' exists on the default volume.
 * If not, then disable the command in the 'File' menu that allows the user to
 * launch the Apple Finder.  Note that this is fairly dumb, in that if it
 * finds ANY file named 'Finder' it thinks it is the one.  Could be made
 * smarter by checking the type and creator bytes for the file, but I
 * didn't feel that I should, in case there is an application by that name.
 */
 
CheckForFinder()
{
     Finfo fndrInfo;
 
     if(GetFInfo("\006Finder", 0, &fndrInfo) < 0)       /* use default volume */
          DisableItem(TFMenu[1], 2);
}
 
 
/*
 * Create our menu bar.
 */
 
FormMenuBar()
{
     int  i;
 
     TFMenu[0] = GetMenu(AppleMenu);
     TFMenu[1] = GetMenu(FileMenu);
     TFMenu[2] = GetMenu(EditMenu);
     TFMenu[3] = GetMenu(SpecialMenu);
     AddResMenu(TFMenu[0], 'DRVR');
     
     for(i = 0;i < NUM_MENUS;++i)
          InsertMenu(TFMenu[i], 0);
 
     DrawMenuBar();
}
 
 
/*
 * This alert takes care of our general error messages.  We can
 * pass up to four Str255 types (pointers to them, that is), and
 * they are combined end to end with a single space between them.
 */
 
GeneralError(str1, str2, str3, str4)
char *str1, *str2, *str3, *str4;                        /* Pascal format */
{
     InitCursor();
     ParamText("\000", "\000", "\000", "\000");
     ParamText(str1, str2, str3, str4);
     StopAlert(GENERAL_ERROR_ALRT, nil);
}
 
 
/*
 * Launch the Apple Finder.  This command is disabled in the menu bar
 * if the program does not find the Finder on the default disk when
 * we boot up and begin execution.
 */
 
GoFinder()
{
     long zeroes = 0;
 
     Launch("\006Finder", &zeroes);
}
 
 
/*
 * Come here when the 'Launch' menu command is given.
 */
 
LaunchAppl()
{
     long        zeroes = 0;
     short       numTypes = 1;
     Ptr         prompt = nil;
     Ptr         fileFilter = nil;
     Handle      hdl;
     SFReply     rec;
     SFTypeList  typeList;
 
     typeList[0] = 'APPL';              /* filter out all but 'APPL' types */
 
     SFGetFile(pass(whereOpen), prompt, fileFilter, numTypes, typeList, nil, &rec);
 
     if(rec.good == 0)                          /* 'Cancel' button hit */
          return;
 
     if(SetVol(nil, rec.vRefNum) < 0) {         /* set the new default volume */
          hdl = GetString(256);
          GeneralError(*hdl, nil, nil, nil);
          return;
     }
 
     Launch(rec.fName, &zeroes);
}
 
 
/*
 * This is the equivalent of Apple Finder v4.1's 'Shut Down' command,
 * in which the diskettes are ejected from the internal & external
 * drives and a system reset is issued.  Any other volumes that happen
 * to be mounted will NOT be handled.
 */
 
ShutDown()
{
     int  vRefNum;
     long freeBytes;
     char volName[256];
 
     GetVInfo(1, volName, &vRefNum, &freeBytes);        /* internal drive */
     Eject(volName, vRefNum);
 
     if(GetVInfo(2, volName, &vRefNum, &freeBytes) != ParamErr) /* external */
          Eject(volName, vRefNum);
 
 
                /* pulse the 68000's RESET pin via a direct assembler call */
#asm
 
     Reset
 
#endasm
}
