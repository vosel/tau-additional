// This source file is part of the 'tau' open source project.
// Copyright (c) 2016, Yuriy Vosel.
// Licensed under Boost Software License.
// See LICENSE.txt for the licence information.

#include <tau/layout_generation/layout_info.h>
#include <tau/util/basic_events_dispatcher.h>
#include <tau/util/boost_asio_server.h>
#include <windows.h>

namespace {
    //This is a helper function that tells us if the KEYEVENTF_EXTENDEDKEY should be set for the given key.
    bool isExtendedKey(char key)
    {
        // NOTE: this method returns true for the subset of the extended keys. It is here for convinience purposes only.
        // For details on this flag, please read the msdn documentation:
        // https://msdn.microsoft.com/en-us/library/windows/desktop/ms646267(v=vs.85).aspx#extended_key_flag
        return
            (key == VK_INSERT) || 
            (key == VK_DELETE) || 
            (key == VK_HOME) || 
            (key == VK_END) || 
            (key == VK_PRIOR) ||        //page up
            (key == VK_NEXT) ||         //page down
            (key == VK_SNAPSHOT) ||     //print screen
            (key == VK_NUMLOCK) ||
            (key == VK_LEFT) ||
            (key == VK_UP) ||
            (key == VK_RIGHT) ||
            (key == VK_DOWN);
    }

    void sendSimpleHotkey(char modifier, char mainKey)
    {
        char extendedKeyFlag = isExtendedKey(mainKey) ? KEYEVENTF_EXTENDEDKEY : 0;

        INPUT keystroke;
        keystroke.type = INPUT_KEYBOARD;
        keystroke.ki.wScan = 0;
        keystroke.ki.time = 0;
        keystroke.ki.dwExtraInfo = 0;

        keystroke.ki.wVk = modifier;
        keystroke.ki.dwFlags = 0; // 0 - key press
        SendInput(1, &keystroke, sizeof(INPUT));

        keystroke.ki.wVk = mainKey;
        keystroke.ki.dwFlags = extendedKeyFlag;
        SendInput(1, &keystroke, sizeof(INPUT));

        keystroke.ki.wVk = mainKey;        
        keystroke.ki.dwFlags = extendedKeyFlag | KEYEVENTF_KEYUP;
        SendInput(1, &keystroke, sizeof(INPUT));

        keystroke.ki.wVk = modifier;
        keystroke.ki.dwFlags = KEYEVENTF_KEYUP;
        SendInput(1, &keystroke, sizeof(INPUT));
    }

    void sendCopyEvent()
    {
        sendSimpleHotkey(VK_CONTROL, VK_INSERT);
        //sendSimpleHotkey(VK_CONTROL, 'c');
    }

    void sendPasteEvent()
    {
        sendSimpleHotkey(VK_SHIFT, VK_INSERT);
        //sendSimpleHotkey(VK_CONTROL, 'v');
    }
};
tau::common::ElementID const COPY_BUTTON("COPY");
tau::common::ElementID const PASTE_BUTTON("PASTE");
class MyEventsDispatcher : public tau::util::BasicEventsDispatcher
{
public:
    MyEventsDispatcher(
        tau::communications_handling::OutgiongPacketsGenerator & outgoingGeneratorToUse): 
            tau::util::BasicEventsDispatcher(outgoingGeneratorToUse)
        {};

    virtual void packetReceived_requestProcessingError(
		std::string const & layoutID, std::string const & additionalData)
    {
        std::cout << "Error received from client:\nLayouID: "
			<< layoutID << "\nError: " << additionalData << "\n";
    }

    virtual void onClientConnected(
        tau::communications_handling::ClientConnectionInfo const & connectionInfo)
    {
        std::cout << "Client connected: remoteAddr: "
            << connectionInfo.getRemoteAddrDump()
            << ", localAddr : "
            << connectionInfo.getLocalAddrDump() << "\n";
    }
    virtual void packetReceived_clientDeviceInfo(tau::communications_handling::ClientDeviceInfo const & info) {
        using namespace tau::layout_generation;
        std::string layoutInfo = LayoutInfo().pushLayoutPage(LayoutPage(tau::common::LayoutPageID("LAYOUT_PAGE_ID"), 
            EvenlySplitLayoutElementsContainer(true)
                .push(ButtonLayoutElement().note("copy").ID(COPY_BUTTON))
                .push(ButtonLayoutElement().note("paste").ID(PASTE_BUTTON)))).getJson();
        sendPacket_resetLayout(layoutInfo);
    };

    virtual void packetReceived_buttonClick(tau::common::ElementID const & buttonID) {
        if (buttonID == COPY_BUTTON) {
            sendCopyEvent();
        } else if (buttonID == PASTE_BUTTON) {
            sendPasteEvent();
        } else {
            std::cout << "Unknown button pressed. This should not happen.\n";
        }
    };
};

int main(int argc, char ** argv)
{
    boost::asio::io_service io_service;
    short port = 12345;
    tau::util::SimpleBoostAsioServer<MyEventsDispatcher>::type s(io_service, port);
    std::cout << "Starting server on port " << port << "...\n";
    s.start();
    std::cout << "Calling io_service.run()\n";
    io_service.run();
    return 0;
}
