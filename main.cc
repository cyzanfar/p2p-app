
#include <unistd.h>

#include <QVBoxLayout>
#include <QApplication>
#include <QDebug>

#include "main.hh"

ChatDialog::ChatDialog()
{
	setWindowTitle("P2Papp");

	// Read-only text box where we display messages from everyone.
	// This widget expands both horizontally and vertically.
	textview = new QTextEdit(this);
	textview->setReadOnly(true);

	// Small text-entry box the user can enter messages.
	// This widget normally expands only horizontally,
	// leaving extra vertical space for the textview widget.
	//
	// You might change this into a read/write QTextEdit,
	// so that the user can easily enter multi-line messages.
	textline = new QLineEdit(this);

	// Lay out the widgets to appear in the main window.
	// For Qt widget and layout concepts see:
	// http://doc.qt.nokia.com/4.7-snapshot/widgets-and-layouts.html
	QVBoxLayout *layout = new QVBoxLayout();
	layout->addWidget(textview);
	layout->addWidget(textline);
	setLayout(layout);

	// Create a UDP network socket
	sock = new NetSocket();
	if (!sock->bind())
		exit(1);

	// Initialize user-defined variables
	nextSeqNum = 1;
	origin = QString(sock->localPort());

	qDebug() << "origin: " << origin;

	// Register a callback on the textline's returnPressed signal
	// so that we can send the message entered by the user.
	connect(textline, SIGNAL(returnPressed()),
		this, SLOT(gotReturnPressed()));

	// callback fired when message is received
	connect(sock, SIGNAL(readyRead()), this, SLOT(readPendingMessages()));

}

void ChatDialog::readPendingMessages()
{
	while (sock->hasPendingDatagrams()) {
		QByteArray datagram;
		datagram.resize(sock->pendingDatagramSize());
		QHostAddress sender;
		quint16 senderPort;

		sock->readDatagram(datagram.data(), datagram.size(),
								&sender, &senderPort);

		qDebug() << "message in sender: " << sender;
		qDebug() << "message in senderPort: " << senderPort;
		qDebug() << "message in datagram: " << datagram.data();

		processMessage(datagram, sender, senderPort);
	}
}
//
void ChatDialog::processMessage(QByteArray datagramReceived, QHostAddress sender, int senderPort)
{
	QMap<QString, QVariant> messageMap;
	QDataStream stream(&datagramReceived,  QIODevice::ReadOnly);

	stream >> messageMap;

	if (messageMap.contains("ChatText")) {
		qDebug() << "message contains chattext" << messageMap.value("ChatText").toString();
		qDebug() << "message contains origin" << messageMap.value("Origin").toString();
		qDebug() << "message contains seqnum" << messageMap.value("SeqNo").toString();

		//If origin already in statusList, seqno+1, if not in list add origin, seqno+1

		textview->append(messageMap.value("ChatText").toString());

		sendStatusMessage(sender, senderPort);
	}
	else if (messageMap.contains("Want")) {
		qDebug() << "message contains want" << messageMap.value("Want").toString();
	}

}

void ChatDialog::gotReturnPressed()
{
	QByteArray buffer;
	QDataStream stream(&buffer,  QIODevice::ReadWrite);
	QMap<QString, QVariant> messageMap;

	// Initially, just echo the string locally.
	// Insert some networking code here...
	qDebug() << "FIX: send message to other peers: " << textline->text();

	// Define message QMap
	messageMap["ChatText"] = textline->text();
	messageMap["Origin"] = origin;
	messageMap["SeqNo"] = nextSeqNum;
	stream << messageMap;

	qDebug() << "message in stream: " << messageMap["ChatText"];

	textview->append(textline->text());

	sendMessage(buffer);

	// Clear the textline to get ready for the next input message.
	textline->clear();
}

void ChatDialog::sendMessage(QByteArray buffer)
{
	qDebug() << "message in buff: " << buffer;
	qDebug() << "message in sock: " << sock;

	QList<int> peerList = sock->PeerList();

	qDebug() << "Peer List: " << peerList;

	int index = rand() % peerList.size();

	qDebug() << "Sending to peer:" << peerList[index];

	sock->writeDatagram(buffer, buffer.size(), QHostAddress::LocalHost, peerList[index]);

}

void ChatDialog::sendStatusMessage(QHostAddress sendto, int port)
{
	QByteArray buffer;
	QDataStream stream(&buffer,  QIODevice::ReadWrite);
	QMap<QString, QMap<QString, quint32>> statusMessage;

	qDebug() << "message in buff: " << buffer;
	qDebug() << "message in sock: " << sock;
	qDebug() << "sending to peer: " << port;

	// Define message QMap
	statusMessage["Want"] = statusList;
	qDebug() << "statusList: " << statusMessage["Want"];

	stream << statusMessage;

	sock->writeDatagram(buffer, buffer.size(), sendto, port);
}

NetSocket::NetSocket()
{
	// Pick a range of four UDP ports to try to allocate by default,
	// computed based on my Unix user ID.
	// This makes it trivial for up to four P2Papp instances per user
	// to find each other on the same host,
	// barring UDP port conflicts with other applications
	// (which are quite possible).
	// We use the range from 32768 to 49151 for this purpose.
	myPortMin = 32768 + (getuid() % 4096)*4;
	myPortMax = myPortMin + 3;
}

QList<int> NetSocket::PeerList()
{
    QList<int> peerList;
	for (int p = myPortMin; p <= myPortMax; p++) {
	    if (this->localPort() != p) {
            peerList.append(p);
	    }
	}
    return peerList;
}

bool NetSocket::bind()
{
	// Try to bind to each of the range myPortMin..myPortMax in turn.
	for (int p = myPortMin; p <= myPortMax; p++) {
		if (QUdpSocket::bind(p)) {
			qDebug() << "bound to UDP port " << p;
			return true;
		}
	}

	qDebug() << "Oops, no ports in my default range " << myPortMin
		<< "-" << myPortMax << " available";
	return false;
}

int main(int argc, char **argv)
{
	// Initialize Qt toolkit
	QApplication app(argc,argv);

	// Create an initial chat dialog window
	ChatDialog dialog;
	dialog.show();


	// Enter the Qt main loop; everything else is event driven
	return app.exec();
}

