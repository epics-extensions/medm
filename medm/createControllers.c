
/*****************************************************************************
 ******				 createControllers.c			******
 *****************************************************************************/

#include "medm.h"


/****************************************************************
 *****    nested objects (not to be put in display list )   *****
/****************************************************************


/***
 *** control element in each controller object
 ***/

static void createDlControl(
  DisplayInfo *displayInfo,
  DlControl *control)
{
  strcpy(control->ctrl,globalResourceBundle.ctrl);
  control->clr = globalResourceBundle.clr;
  control->bclr = globalResourceBundle.bclr;
}






/***
 *** Choice Button
 ***/

DlElement *createDlChoiceButton(
  DisplayInfo *displayInfo)
{
  DlChoiceButton *dlChoiceButton;
  DlElement *dlElement;

  dlChoiceButton = (DlChoiceButton *) malloc(sizeof(DlChoiceButton));
  createDlObject(displayInfo,&(dlChoiceButton->object));
  createDlControl(displayInfo,&(dlChoiceButton->control));
  dlChoiceButton->clrmod = globalResourceBundle.clrmod;
  dlChoiceButton->stacking = globalResourceBundle.stacking;


  dlElement = (DlElement *) malloc(sizeof(DlElement));
  dlElement->type = DL_ChoiceButton;
  dlElement->structure.choiceButton = dlChoiceButton;
  dlElement->next = NULL;
  dlElement->prev = displayInfo->dlElementListTail;
  displayInfo->dlElementListTail->next = dlElement;
  displayInfo->dlElementListTail = dlElement;
  dlElement->dmExecute = (void(*)())executeDlChoiceButton;
  dlElement->dmWrite = (void(*)())writeDlChoiceButton;

  return(dlElement);
}



/***
 *** Message Button
 ***/

DlElement *createDlMessageButton(
  DisplayInfo *displayInfo)
{
  DlMessageButton *dlMessageButton;
  DlElement *dlElement;

  dlMessageButton = (DlMessageButton *) malloc(sizeof(DlMessageButton));
  createDlObject(displayInfo,&(dlMessageButton->object));
  createDlControl(displayInfo,&(dlMessageButton->control));
  strcpy(dlMessageButton->label,globalResourceBundle.messageLabel);
  strcpy(dlMessageButton->press_msg,globalResourceBundle.press_msg);
  strcpy(dlMessageButton->release_msg,globalResourceBundle.release_msg);

  dlMessageButton->clrmod = globalResourceBundle.clrmod;


  dlElement = (DlElement *) malloc(sizeof(DlElement));
  dlElement->type = DL_MessageButton;
  dlElement->structure.messageButton = dlMessageButton;
  dlElement->next = NULL;
  dlElement->prev = displayInfo->dlElementListTail;
  displayInfo->dlElementListTail->next = dlElement;
  displayInfo->dlElementListTail = dlElement;
  dlElement->dmExecute = (void(*)())executeDlMessageButton;
  dlElement->dmWrite = (void(*)())writeDlMessageButton;

  return(dlElement);
}


/***
 *** Valuator (Scale)
 ***/


DlElement *createDlValuator(
  DisplayInfo *displayInfo)
{
  DlValuator *dlValuator;
  DlElement *dlElement;

  dlValuator = (DlValuator *) malloc(sizeof(DlValuator));
  createDlObject(displayInfo,&(dlValuator->object));
  createDlControl(displayInfo,&(dlValuator->control));

  dlValuator->label = globalResourceBundle.label;
  dlValuator->clrmod = globalResourceBundle.clrmod;
  dlValuator->direction = globalResourceBundle.direction;
  dlValuator->dPrecision = globalResourceBundle.dPrecision;

/* private run-time valuator field */
  dlValuator->enableUpdates = True;
  dlValuator->dragging = False;


  dlElement = (DlElement *) malloc(sizeof(DlElement));
  dlElement->type = DL_Valuator;
  dlElement->structure.valuator = dlValuator;
  dlElement->next = NULL;
  dlElement->prev = displayInfo->dlElementListTail;
  displayInfo->dlElementListTail->next = dlElement;
  displayInfo->dlElementListTail = dlElement;
  dlElement->dmExecute = (void(*)())executeDlValuator;
  dlElement->dmWrite = (void(*)())writeDlValuator;

  return(dlElement);
}







/***
 *** Text Entry
 ***/

DlElement *createDlTextEntry(
  DisplayInfo *displayInfo)
{
  DlTextEntry *dlTextEntry;
  DlElement *dlElement;

  dlTextEntry = (DlTextEntry *) malloc(sizeof(DlTextEntry));
  createDlObject(displayInfo,&(dlTextEntry->object));
  createDlControl(displayInfo,&(dlTextEntry->control));
  dlTextEntry->clrmod = globalResourceBundle.clrmod;
  dlTextEntry->format = globalResourceBundle.format;


  dlElement = (DlElement *) malloc(sizeof(DlElement));
  dlElement->type = DL_TextEntry;
  dlElement->structure.textEntry = dlTextEntry;
  dlElement->next = NULL;
  dlElement->prev = displayInfo->dlElementListTail;
  displayInfo->dlElementListTail->next = dlElement;
  displayInfo->dlElementListTail = dlElement;
  dlElement->dmExecute = (void(*)())executeDlTextEntry;
  dlElement->dmWrite = (void(*)())writeDlTextEntry;

  return(dlElement);
}



/***
 *** Menu
 ***/

DlElement *createDlMenu(
  DisplayInfo *displayInfo)
{
  DlMenu *dlMenu;
  DlElement *dlElement;

  dlMenu = (DlMenu *) malloc(sizeof(DlMenu));
  createDlObject(displayInfo,&(dlMenu->object));
  createDlControl(displayInfo,&(dlMenu->control));
  dlMenu->clrmod = globalResourceBundle.clrmod;

  dlElement = (DlElement *) malloc(sizeof(DlElement));
  dlElement->type = DL_Menu;
  dlElement->structure.menu = dlMenu;
  dlElement->next = NULL;
  dlElement->prev = displayInfo->dlElementListTail;
  displayInfo->dlElementListTail->next = dlElement;
  displayInfo->dlElementListTail = dlElement;
  dlElement->dmExecute = (void(*)())executeDlMenu;
  dlElement->dmWrite = (void(*)())writeDlMenu;

  return(dlElement);
}



