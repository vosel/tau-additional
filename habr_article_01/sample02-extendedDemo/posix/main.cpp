// This source file is part of the 'tau' open source project.
// Copyright (c) 2016, Yuriy Vosel.
// Licensed under Boost Software License.
// See LICENSE.txt for the licence information.

#include <tau/layout_generation/layout_info.h>
#include <tau/util/basic_events_dispatcher.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <tau/communications_handling/outgiong_packets_generator.h>
#include <tau/communications_handling/incoming_data_stream_parser.h>


namespace {
    std::string const INITIAL_TEXT_VALUE("initial text");    
    tau::common::LayoutID const LAYOUT_ID("SAMPLE_LAYOUT_ID");
    tau::common::LayoutPageID const LAYOUT_PAGE1_ID("LAYOUT_PAGE_1");
    tau::common::LayoutPageID const LAYOUT_PAGE2_ID("LAYOUT_PAGE_2");
    tau::common::ElementID const BUTTON_WITH_NOTE_TO_REPLACE_ID("BUTTON_WITH_NOTE_TO_REPLACE");
    tau::common::ElementID const BUTTON_TO_RESET_VALUES_ID("BUTTON_TO_RESET_NOTES");
    tau::common::ElementID const BUTTON_TO_PAGE_1_ID("BUTTON_TO_PG1");
    tau::common::ElementID const BUTTON_TO_PAGE_2_ID("BUTTON_TO_PG2");
    tau::common::ElementID const BUTTON_1_ID("BUTTON_1");
    tau::common::ElementID const BUTTON_2_ID("BUTTON_2");
    tau::common::ElementID const BUTTON_3_ID("BUTTON_3");
    tau::common::ElementID const BUTTON_4_ID("BUTTON_4");
    tau::common::ElementID const TEXT_INPUT_ID("TEXT_INPUT");
    tau::common::ElementID const BOOL_INPUT_ID("BOOL_INPUT");
    tau::common::ElementID const LABEL_ON_PAGE2_ID("LABEL_ON_PAGE2");
};

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
    virtual void packetReceived_clientDeviceInfo(
        tau::communications_handling::ClientDeviceInfo const & info)
    {
        using namespace tau::layout_generation;
        std::cout << "Received client information packet\n";
        LayoutInfo resultLayout;
        resultLayout.pushLayoutPage(LayoutPage(LAYOUT_PAGE1_ID, 
            EvenlySplitLayoutElementsContainer(true)
                .push(EvenlySplitLayoutElementsContainer(false)
                    .push(BooleanInputLayoutElement(true).note(INITIAL_TEXT_VALUE).ID(BOOL_INPUT_ID))
                    .push(ButtonLayoutElement().note(INITIAL_TEXT_VALUE)
                        .ID(BUTTON_WITH_NOTE_TO_REPLACE_ID)))
                .push(TextInputLayoutElement().ID(TEXT_INPUT_ID).initialValue(INITIAL_TEXT_VALUE))
                .push(EmptySpace())
                .push(EmptySpace())
                .push(EmptySpace())
                .push(EvenlySplitLayoutElementsContainer(false)
                    .push(ButtonLayoutElement().note("reset notes").ID(BUTTON_TO_RESET_VALUES_ID))
                    .push(EmptySpace())
                    .push(ButtonLayoutElement().note("go to page 2").ID(BUTTON_TO_PAGE_2_ID)
                        .switchToAnotherLayoutPageOnClick(LAYOUT_PAGE2_ID))
                    )
            )
        );
        resultLayout.pushLayoutPage(LayoutPage(LAYOUT_PAGE2_ID, 
            EvenlySplitLayoutElementsContainer(true)
                .push(EvenlySplitLayoutElementsContainer(false)
                    .push(ButtonLayoutElement().note("1").ID(BUTTON_1_ID))
                    .push(ButtonLayoutElement().note("2").ID(BUTTON_2_ID)))
                .push(EvenlySplitLayoutElementsContainer(false)
                    .push(ButtonLayoutElement().note("3").ID(BUTTON_3_ID))
                    .push(ButtonLayoutElement().note("4").ID(BUTTON_4_ID)))
                .push(EvenlySplitLayoutElementsContainer(true)
                    .push(LabelElement("").ID(LABEL_ON_PAGE2_ID))
                    .push(ButtonLayoutElement().note("back to page 1").ID(BUTTON_TO_PAGE_1_ID)))
        ));
        resultLayout.setStartLayoutPage(LAYOUT_PAGE1_ID);
        sendPacket_resetLayout(resultLayout.getJson());
    }
    virtual void packetReceived_buttonClick(
        tau::common::ElementID const & buttonID)
    {
        std::cout << "event: buttonClick, id=" << buttonID << "\n";
        if (buttonID == BUTTON_TO_RESET_VALUES_ID) {
            sendPacket_updateTextValue(TEXT_INPUT_ID, INITIAL_TEXT_VALUE);
        } else if (buttonID == BUTTON_TO_PAGE_1_ID) {
            sendPacket_changeShownLayoutPage(LAYOUT_PAGE1_ID);
        } else if (buttonID == BUTTON_1_ID) {
            sendPacket_changeElementNote(LABEL_ON_PAGE2_ID, "Button 1 pressed");
        } else if (buttonID == BUTTON_2_ID) {
            sendPacket_changeElementNote(LABEL_ON_PAGE2_ID, "Button 2 pressed");
        } else if (buttonID == BUTTON_3_ID) {
            sendPacket_changeElementNote(LABEL_ON_PAGE2_ID, "Button 3 pressed");
        } else if (buttonID == BUTTON_4_ID) {
            sendPacket_changeElementNote(LABEL_ON_PAGE2_ID, "Button 4 pressed");
        }
    }
    virtual void packetReceived_layoutPageSwitched(
        tau::common::LayoutPageID const & newActiveLayoutPageID)
    {
        std::cout << "event: layoutPageSwitch, id=" << newActiveLayoutPageID << "\n";
    }
    virtual void packetReceived_boolValueUpdate(
        tau::common::ElementID const & inputBoxID,
        bool new_value, bool is_automatic_update)
    {
        std::cout << "event: boolValueUpdate, id="
            << inputBoxID << ", value=" << new_value << "\n";
    }
    virtual void packetReceived_textValueUpdate(
        tau::common::ElementID const & inputBoxID,
        std::string const & new_value, bool is_automatic_update)
    {
        std::cout << "event: textValueUpdate, id="
            << inputBoxID << ",\n\tvalue=" << new_value << "\n";
        sendPacket_changeElementNote(BOOL_INPUT_ID, new_value);
        sendPacket_changeElementNote(BUTTON_WITH_NOTE_TO_REPLACE_ID, new_value);
    }
};
class MyOutgoingPacketsGenerator : 
    public tau::communications_handling::OutgiongPacketsGenerator
{
    int m_socketHandle;
public:
    MyOutgoingPacketsGenerator(int output_socket_handle): m_socketHandle(output_socket_handle)
    {};
    virtual void sendData(std::string const & data) {
        write(m_socketHandle, data.c_str(), data.size());
    }
};
std::string getAddrString(sockaddr_in const & toConvert) {
	int buflen = INET_ADDRSTRLEN;
	char buf[buflen];
	return inet_ntop(AF_INET, &(toConvert.sin_addr), buf, buflen);
}

int main(int argc, char ** argv)
{
    int socket_handle = socket(AF_INET, SOCK_STREAM, 0);
    if (socket_handle == -1) {
        std::cerr << "Can't create the socket. Exiting.\n";
        return -1;
    }
    std::cout << "Socket created. ";
    int listenPort = 12345;
    sockaddr_in serverAddr;
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = INADDR_ANY;
    serverAddr.sin_port = htons(listenPort);

    if (bind(socket_handle, (sockaddr*)(&serverAddr), sizeof(serverAddr)) < 0) {
        std::cerr << "Can't bind the socket. Exiting.\n";
        return -2;
    }
    std::cout << "bind() succeded. Starting to listen on the socket (port " << listenPort << ")...\n";
    listen(socket_handle, 5);
    sockaddr_in client;
    int client_socket_size = sizeof(sockaddr_in);
    int client_handle = accept(socket_handle, (sockaddr*)(&client), (socklen_t*)&client_socket_size);
    if (client_handle < 0) {
        std::cerr << "Accept failed. Exiting.\n";
        return -3;
    }
    std::cout << "Connection accepted.";
    int read_bufSize;
    const int BUFSIZE = 1024;
    char buffer[BUFSIZE];
    MyOutgoingPacketsGenerator outgoingPacketsGenerator(client_handle);
    MyEventsDispatcher myServerLogic(outgoingPacketsGenerator);
    tau::communications_handling::IncomingDataStreamParser incomingDataStreamParser;
    
    tau::communications_handling::ClientConnectionInfo connectionInfo(getAddrString(client), ntohs(client.sin_port), getAddrString(serverAddr), ntohs(serverAddr.sin_port));
    myServerLogic.onClientConnected(connectionInfo); //Notify the user code about the connection details
    
    while((read_bufSize = recv(client_handle, buffer, BUFSIZE, 0)) > 0) {
        std::string received_data(buffer, read_bufSize);
        incomingDataStreamParser.newData(
                received_data, 
                myServerLogic.getIncomingPacketsHandler(), 
                myServerLogic.getCommunicationIssuesHandler()
            );
    }
    myServerLogic.onConnectionClosed();
    if (read_bufSize == 0) {
        std::cout << "Client disconnected. Exiting.\n";
    } else if (read_bufSize == -1) {
        std::cout << "recv() failed. Exiting.\n";
        return -4;
    }
    return 0;
}
