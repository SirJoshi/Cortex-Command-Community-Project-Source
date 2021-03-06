#ifndef _GUIEVENT_
#define _GUIEVENT_

//////////////////////////////////////////////////////////////////////////////////////////
// File:            GUIEvent.h
//////////////////////////////////////////////////////////////////////////////////////////
// Description:     GUIEvent class
// Project:         GUI Library
// Author(s):       Jason Boettcher
//                  jackal@shplorb.com
//                  www.shplorb.com/~jackal


namespace RTE
{


//////////////////////////////////////////////////////////////////////////////////////////
// Class:           GUIEvent
//////////////////////////////////////////////////////////////////////////////////////////
// Description:     A class to hold event information
// Parent(s):       None.
// Class history:   1/7/2004 GUIEvent Created.

class GUIEvent {

//////////////////////////////////////////////////////////////////////////////////////////
// Public member variable, method and friend function declarations

public:

    // Event Types
    enum {
        Command = 0,
        Notification
    } EventType;


//////////////////////////////////////////////////////////////////////////////////////////
// Constructor:     GUIEvent
//////////////////////////////////////////////////////////////////////////////////////////
// Description:     Constructor method used to instantiate a GUIEvent object in system
//                  memory.
// Arguments:       None.

    GUIEvent();


//////////////////////////////////////////////////////////////////////////////////////////
// Constructor:     GUIEvent
//////////////////////////////////////////////////////////////////////////////////////////
// Description:     Constructor method used to instantiate a GUIEvent object in system
//                  memory.
// Arguments:       Control, Event type, Msg, Data.

    GUIEvent(GUIControl *Control, int Type, int Msg, int Data);


//////////////////////////////////////////////////////////////////////////////////////////
// Method:          GetType
//////////////////////////////////////////////////////////////////////////////////////////
// Description:     Gets the event type
// Arguments:       None.

    int GetType();


//////////////////////////////////////////////////////////////////////////////////////////
// Method:          GetMsg
//////////////////////////////////////////////////////////////////////////////////////////
// Description:     Gets the msg.
// Arguments:       None.

    int GetMsg();


//////////////////////////////////////////////////////////////////////////////////////////
// Method:          GetData
//////////////////////////////////////////////////////////////////////////////////////////
// Description:     Gets the data.
// Arguments:       None.

    int GetData();


//////////////////////////////////////////////////////////////////////////////////////////
// Method:          GetControl
//////////////////////////////////////////////////////////////////////////////////////////
// Description:     Gets the event control.
// Arguments:       None.

    GUIControl *GetControl();


//////////////////////////////////////////////////////////////////////////////////////////
// Private member variable and method declarations

private:

    GUIControl        *m_Control;
    int                m_Type;
    int                m_Msg;
    int                m_Data;

};


}; // namespace RTE


#endif  // _GUIEVENT_