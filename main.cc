
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

		processMessage(datagram);

	}
}
//
void ChatDialog::processMessage(QByteArray datagramReceived)
{
	QMap<QString, QVariant> messageMap;

	QDataStream stream(&datagramReceived,  QIODevice::ReadOnly);

	stream >> messageMap;

	if (messageMap.contains("ChatText")) {
		qDebug() << "message contains chattext" << messageMap.value("ChatText").toString();

//		textview->append(messageMap.value("ChatText"));

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

	sock->writeDatagram(buffer, buffer.size(), QHostAddress::LocalHost, 36769);

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

