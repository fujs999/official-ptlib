/*
 * main.h
 *
 * PWLib application header file for vxmltest
 *
 * Copyright 2002 Equivalence
 *
 */

#ifndef _Vxmltest_MAIN_H
#define _Vxmltest_MAIN_H

#include <ptlib/pprocess.h>

class PVXMLSession;
class PVideoInputDevice;
class PVideoOutputDevice;


class TestInstance
{
#if P_VXML
  public:
    TestInstance();
    ~TestInstance();

    bool Initialise(unsigned instance, const PArgList & args);
    void SendInput(const PString & digits);
    void SetVar(const PString & name, const PString & value) { m_vxml->SetVar(name, value); }
    PString GetVar(const PString & name) { return m_vxml->GetVar(name); }

  protected:
    unsigned             m_instance;
    PSoundChannel      * m_player;
    PSoundChannel      * m_recorder;
    PVideoInputDevice  * m_grabber;
    PVideoOutputDevice * m_preview;
    PVideoOutputDevice * m_viewer;
    PVXMLSession       * m_vxml;

    PThread * m_playerThread;
    void PlayAudio();

    PThread * m_recorderThread;
    void RecordAudio();

#if P_VXML_VIDEO
    PThread * m_videoSenderThread;
    void CopyVideoSender();

    PThread * m_videoReceiverThread;
    void CopyVideoReceiver();
#endif // P_VXML_VIDEO
#endif // P_VXML
};


class VxmlTest : public PProcess
{
  PCLASSINFO(VxmlTest, PProcess)

  public:
    VxmlTest();
    void Main();

  protected:
    PDECLARE_NOTIFIER(PCLI::Arguments, VxmlTest, SimulateInput);
    PDECLARE_NOTIFIER(PCLI::Arguments, VxmlTest, SetVar);
    PDECLARE_NOTIFIER(PCLI::Arguments, VxmlTest, GetVar);
    std::vector<TestInstance> m_tests;
};


#endif  // _Vxmltest_MAIN_H


// End of File ///////////////////////////////////////////////////////////////
