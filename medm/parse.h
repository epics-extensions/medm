
/****************************************************************************
 ***                             parse.h                                  ***
 ****************************************************************************/

#ifndef __PARSE_H__
#define __PARSE_H__

/*
 * types for parsing
 */
typedef enum 
   { T_WORD, T_EQUAL, T_QUOTE, T_LEFT_BRACE, T_RIGHT_BRACE, T_EOF} TOKEN;


/*
 * position element at tail of display list or composite list...
 *   (referenced in parse*.c files)
 */

#define POSITION_ELEMENT_ON_LIST() \
  if (dlComposite != (DlComposite *)NULL) {				\
   dlElement->prev = (DlElement *)(dlComposite->dlElementListTail);	\
   if ( ((DlElement *)dlComposite->dlElementListHead)->next == NULL) {	\
     ((DlElement *)dlComposite->dlElementListHead)->next = dlElement;	\
     dlComposite->dlElementListTail = (XtPointer)dlElement;		\
   } else {								\
     ((DlElement *)dlComposite->dlElementListTail)->next = dlElement;	\
   }									\
   dlComposite->dlElementListTail = (XtPointer)dlElement;		\
  } else {								\
   dlElement->prev = displayInfo->dlElementListTail;			\
   ((DlElement *)displayInfo->dlElementListTail)->next = dlElement;	\
   displayInfo->dlElementListTail = dlElement;				\
  }


#endif  /* __PARSE_H__ */
