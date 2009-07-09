/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2009 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** Commercial Usage
**
** Licensees holding valid Qt Commercial licenses may use this file in
** accordance with the Qt Commercial License Agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Nokia.
**
** GNU Lesser General Public License Usage
**
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** If you are unsure which license is appropriate for your use, please
** contact the sales department at http://www.qtsoftware.com/contact.
**
**************************************************************************/

#include <qglobal.h>

#include <QtCore/QDateTime>
#include <QtCore/QDebug>
#include <QtCore/QDir>
#include <QtCore/QFile>
#include <QtCore/QFileInfo>
#include <QtCore/QHash>
#include <QtCore/QLinkedList>
#include <QtCore/QLocale>
#include <QtCore/QMap>
#include <QtCore/QMetaObject>
#include <QtCore/QMetaProperty>
#include <QtCore/QMetaEnum>
#include <QtCore/QModelIndex>
#include <QtCore/QObject>
#include <QtCore/QPointer>
#include <QtCore/QString>
#include <QtCore/QTextCodec>
#include <QtCore/QVector>
#include <QtCore/QTextStream>
#include <QtCore/QPoint>
#include <QtCore/QSize>
#include <QtCore/QRect>
#include <QtCore/QPointF>
#include <QtCore/QSizeF>
#include <QtCore/QRectF>

#if QT_VERSION >= 0x040500
#include <QtCore/QSharedPointer>
#include <QtCore/QSharedDataPointer>
#include <QtCore/QSharedData>
#include <QtCore/QWeakPointer>
#endif

int qtGhVersion = QT_VERSION;

#ifndef USE_QT_GUI
#   ifdef QT_GUI_LIB
#        define USE_QT_GUI 1
#   endif
#endif

#if USE_QT_GUI
#   include <QtGui/QApplication>
#   include <QtGui/QImage>
#   include <QtGui/QPixmap>
#   include <QtGui/QWidget>
#   include <QtGui/QFont>
#   include <QtGui/QColor>
#   include <QtGui/QKeySequence>
#   include <QtGui/QSizePolicy>
#endif

#ifdef Q_OS_WIN
#    include <windows.h>
#endif

#include <list>
#include <map>
#include <string>
#include <set>
#include <vector>

/*!
  \class QDumper
  \brief Helper class for producing "nice" output in Qt Creator's debugger.

  \internal

  The whole "custom dumper" implementation is currently far less modular
  than it could be. But as the code is still in a flux, making it nicer
  from a pure archtectural point of view seems still be a waste of resources.

  Some hints:

  New dumpers for non-templated classes should be mentioned in
  \c{qDumpObjectData440()} in the  \c{protocolVersion == 1} branch.

  Templated classes need extra support on the IDE level
  (see plugins/debugger/gdbengine.cpp) and should not be mentiond in
  \c{qDumpObjectData440()}.

  In any case, dumper processesing should end up in
  \c{handleProtocolVersion2and3()} and needs an entry in the big switch there.

  Next step is to create a suitable \c{static void qDumpFoo(QDumper &d)}
  function. At the bare minimum it should contain something like this:


  \c{
    const Foo &foo = *reinterpret_cast<const Foo *>(d.data);

    d.putItem("value", ...);
    d.putItem("type", "Foo");
    d.putItem("numchild", "0");
  }


  'd.putItem(name, value)' roughly expands to:
        d.put((name)).put("=\"").put(value).put("\"";

  Useful (i.e. understood by the IDE) names include:

  \list
    \o "name" shows up in the first column in the Locals&Watchers view.
    \o "value" shows up in the second column.
    \o "valueencoded" should be set to "1" if the value is base64 encoded.
        Always base64-encode values that might use unprintable or otherwise
        "confuse" the protocol (like spaces and quotes). [A-Za-z0-9] is "safe".
        A value of "3" is used for base64-encoded UCS4, "2" denotes
        base64-encoded UTF16.
    \o "numchild" return the number of children in the view. Effectively, only
        0 and != 0 will be used, so don't try too hard to get the number right.
  \endlist

  If the current item has children, it might be queried to produce information
  about these children. In this case the dumper should use something like this:

  \c{
    if (d.dumpChildren) {
        d.beginChildren();
            [...]
        d.endChildren();
   }

  */

#undef NS
#ifdef QT_NAMESPACE
#   define STRINGIFY0(s) #s
#   define STRINGIFY1(s) STRINGIFY0(s)
#   define NS STRINGIFY1(QT_NAMESPACE) "::"
#   define NSX "'" STRINGIFY1(QT_NAMESPACE) "::"
#   define NSY "'"
#else
#   define NS ""
#   define NSX ""
#   define NSY ""
#endif

#if defined(QT_BEGIN_NAMESPACE)
QT_BEGIN_NAMESPACE
#endif

struct Sender { QObject *sender; int signal; int ref; };

const char *stdStringTypeC = "std::basic_string<char,std::char_traits<char>,std::allocator<char> >";
const char *stdWideStringTypeUShortC = "std::basic_string<unsigned short,std::char_traits<unsigned short>,std::allocator<unsigned short> >";

#if QT_VERSION < 0x040600
    struct Connection
    {
        QObject *receiver;
        int method;
        uint connectionType : 3; // 0 == auto, 1 == direct, 2 == queued, 4 == blocking
        QBasicAtomicPointer<int> argumentTypes;
    };

    typedef QList<Connection> ConnectionList;
    typedef QList<Sender> SenderList;

    const Connection &connectionAt(const ConnectionList &l, int i) { return l.at(i); }
    const QObject *senderAt(const SenderList &l, int i) { return l.at(i).sender; }
    int signalAt(const SenderList &l, int i) { return l.at(i).signal; }
#endif

#if QT_VERSION >= 0x040600
    struct Connection
    {
        QObject *sender;
        QObject *receiver;
        int method;
        uint connectionType : 3; // 0 == auto, 1 == direct, 2 == queued, 4 == blocking
        QBasicAtomicPointer<int> argumentTypes;
        //senders linked list
        //Connection *next;
        //Connection **prev;
    };

    typedef QList<Connection *> ConnectionList;
    typedef ConnectionList SenderList;

    const Connection &connectionAt(const ConnectionList &l, int i) { return *l.at(i); }
    const QObject *senderAt(const SenderList &l, int i) { return l.at(i)->sender; }
    // FIXME: 'method' is wrong
    int signalAt(const SenderList &l, int i) { return l.at(i)->method; }
#endif

class ObjectPrivate : public QObjectData
{
public:
    ObjectPrivate() {}
    virtual ~ObjectPrivate() {}

    QList<QObject *> pendingChildInsertedEvents;
    void *threadData;
    void *currentSender;
    void *currentChildBeingDeleted;

    QList<QPointer<QObject> > eventFilters;

    void *extraData;
    mutable quint32 connectedSignals;

    QString objectName;

    void *connectionLists;
    SenderList senders;
    int *deleteWatch;
};


#if defined(QT_BEGIN_NAMESPACE)
QT_END_NAMESPACE
#endif


// This can be mangled typenames of nested templates, each char-by-char
// comma-separated integer list...
// The output buffer.
#ifdef MACROSDEBUG
    Q_DECL_EXPORT char xDumpInBuffer[10000];
    Q_DECL_EXPORT char xDumpOutBuffer[1000000];
    #define inBuffer xDumpInBuffer
    #define outBuffer xDumpOutBuffer
#else
    Q_DECL_EXPORT char qDumpInBuffer[10000];
    Q_DECL_EXPORT char qDumpOutBuffer[1000000];
    #define inBuffer qDumpInBuffer
    #define outBuffer qDumpOutBuffer
#endif

namespace {

static QByteArray strPtrConst = "* const";

static bool isPointerType(const QByteArray &type)
{
    return type.endsWith('*') || type.endsWith(strPtrConst);
}

static QByteArray stripPointerType(const QByteArray &_type)
{
    QByteArray type = _type;
    if (type.endsWith('*'))
        type.chop(1);
    if (type.endsWith(strPtrConst))
        type.chop(7);
    if (type.endsWith(' '))
        type.chop(1);
    return type;
}

// This is used to abort evaluation of custom data dumpers in a "coordinated"
// way. Abortion will happen at the latest when we try to access a non-initialized
// non-trivial object, so there is no way to prevent this from occuring at all
// conceptionally.  Ideally, if there is API to check memory access, it should
// be used to terminate nicely, especially with CDB.
// 1) Gdb will catch SIGSEGV and return to the calling frame.
//    This is just fine provided we only _read_ memory in the custom handlers
//    below.
// 2) For MSVC/CDB, exceptions must be handled in the dumper, which is
//    achieved using __try/__except. The exception will be reported in the
//    debugger, which will then execute a 'gN' command, passing handling back
//    to the __except clause.

volatile int qProvokeSegFaultHelper;

static const void *addOffset(const void *p, int offset)
{
    return offset + reinterpret_cast<const char *>(p);
}

static const void *skipvtable(const void *p)
{
    return sizeof(void *) + reinterpret_cast<const char *>(p);
}

static const void *deref(const void *p)
{
    return *reinterpret_cast<const char* const*>(p);
}

static const void *dfunc(const void *p)
{
    return deref(skipvtable(p));
}

static bool isEqual(const char *s, const char *t)
{
    return qstrcmp(s, t) == 0;
}

static bool startsWith(const char *s, const char *t)
{
    while (char c = *t++)
        if (c != *s++)
            return false;
    return true;
}

// Check memory for read access and provoke segfault if nothing else helps.
// On Windows, try to be less crash-prone by checking memory using WinAPI

#ifdef Q_OS_WIN
#    define qCheckAccess(d) do { if (IsBadReadPtr(d, 1)) return; qProvokeSegFaultHelper = *(char*)d; } while (0)
#    define qCheckPointer(d) do { if (d && IsBadReadPtr(d, 1)) return; if (d) qProvokeSegFaultHelper = *(char*)d; } while (0)
#else
#    define qCheckAccess(d) do { qProvokeSegFaultHelper = *(char*)d; } while (0)
#    define qCheckPointer(d) do { if (d) qProvokeSegFaultHelper = *(char*)d; } while (0)
#endif

#ifdef QT_NAMESPACE
const char *stripNamespace(const char *type)
{
    static const size_t nslen = strlen(NS);
    return startsWith(type, NS) ? type + nslen : type;
}
#else
inline const char *stripNamespace(const char *type)
{
    return type;
}
#endif

static bool isSimpleType(const char *type)
{
    switch (type[0]) {
        case 'c':
            return isEqual(type, "char");
        case 'd':
            return isEqual(type, "double");
        case 'f':
            return isEqual(type, "float");
        case 'i':
            return isEqual(type, "int");
        case 'l':
            return isEqual(type, "long") || startsWith(type, "long ");
        case 's':
            return isEqual(type, "short") || startsWith(type, "short ")
                || isEqual(type, "signed") || startsWith(type, "signed ");
        case 'u':
            return isEqual(type, "unsigned") || startsWith(type, "unsigned ");
    }
    return false;
}

#if 0
static bool isStringType(const char *type)
{
    return isEqual(type, NS"QString")
        || isEqual(type, NS"QByteArray")
        || isEqual(type, "std::string")
        || isEqual(type, "std::wstring")
        || isEqual(type, "wstring");
}
#endif

static bool isMovableType(const char *type)
{
    if (isPointerType(type))
        return true;

    if (isSimpleType(type))
        return true;

    type = stripNamespace(type);

    switch (type[1]) {
        case 'B':
            return isEqual(type, "QBrush")
                || isEqual(type, "QBitArray")
                || isEqual(type, "QByteArray") ;
        case 'C':
            return isEqual(type, "QCustomTypeInfo")
                || isEqual(type, "QChar");
        case 'D':
            return isEqual(type, "QDate")
                || isEqual(type, "QDateTime");
        case 'F':
            return isEqual(type, "QFileInfo")
                || isEqual(type, "QFixed")
                || isEqual(type, "QFixedPoint")
                || isEqual(type, "QFixedSize");
        case 'H':
            return isEqual(type, "QHashDummyValue");
        case 'I':
            return isEqual(type, "QIcon")
                || isEqual(type, "QImage");
        case 'L':
            return isEqual(type, "QLine")
                || isEqual(type, "QLineF")
                || isEqual(type, "QLatin1Char")
                || isEqual(type, "QLocal");
        case 'M':
            return isEqual(type, "QMatrix")
                || isEqual(type, "QModelIndex");
        case 'P':
            return isEqual(type, "QPoint")
                || isEqual(type, "QPointF")
                || isEqual(type, "QPen")
                || isEqual(type, "QPersistentModelIndex");
        case 'R':
            return isEqual(type, "QResourceRoot")
                || isEqual(type, "QRect")
                || isEqual(type, "QRectF")
                || isEqual(type, "QRegExp");
        case 'S':
            return isEqual(type, "QSize")
                || isEqual(type, "QSizeF")
                || isEqual(type, "QString");
        case 'T':
            return isEqual(type, "QTime")
                || isEqual(type, "QTextBlock");
        case 'U':
            return isEqual(type, "QUrl");
        case 'V':
            return isEqual(type, "QVariant");
        case 'X':
            return isEqual(type, "QXmlStreamAttribute")
                || isEqual(type, "QXmlStreamNamespaceDeclaration")
                || isEqual(type, "QXmlStreamNotationDeclaration")
                || isEqual(type, "QXmlStreamEntityDeclaration");
    }
    return false;
}

struct QDumper
{
    explicit QDumper();
    ~QDumper();

    // direct write to the output
    QDumper &put(long c);
    QDumper &put(int i);
    QDumper &put(double d);
    QDumper &put(float d);
    QDumper &put(unsigned long c);
    QDumper &put(unsigned int i);
    QDumper &put(const void *p);
    QDumper &put(qulonglong c);
    QDumper &put(long long c);
    QDumper &put(const char *str);
    QDumper &put(const QByteArray &ba);
    QDumper &put(const QString &str);
    QDumper &put(char c);

    // convienience functions for writing key="value" pairs:
    template <class Value>
    void putItem(const char *name, const Value &value)
    {
        putCommaIfNeeded();
        put(name).put('=').put('"').put(value).put('"');
    }

    // convienience functions for writing typical properties.
    // roughly equivalent to
    //   beginHash();
    //      putItem("name", name);
    //      putItem("value", value);
    //      putItem("type", NS"QString");
    //      putItem("numchild", "0");
    //      putItem("valueencoded", "2");
    //   endHash();
    void putHash(const char *name, const QString &value);
    void putHash(const char *name, const QByteArray &value);
    void putHash(const char *name, int value);
    void putHash(const char *name, long value);
    void putHash(const char *name, bool value);
    void putHash(const char *name, QChar value);

    void beginHash(); // start of data hash output
    void endHash(); // start of data hash output

    void beginChildren(); // start of children list
    void endChildren(); // end of children list

    void beginItem(const char *name); // start of named item, ready to accept value
    void endItem(); // end of named item, used after value output is complete

    // convienience for putting "<n items>"
    void putItemCount(const char *name, int count);
    void putCommaIfNeeded();
    // convienience function for writing the last item of an abbreviated list
    void putEllipsis();
    void disarm();

    void putBase64Encoded(const char *buf, int n);
    void checkFill();

    // the dumper arguments
    int protocolVersion;   // dumper protocol version
    int token;             // some token to show on success
    const char *outertype; // object type
    const char *iname;     // object name used for display
    const char *exp;       // object expression
    const char *innertype; // 'inner type' for class templates
    const void *data;      // pointer to raw data
    bool dumpChildren;     // do we want to see children?

    // handling of nested templates
    void setupTemplateParameters();
    enum { maxTemplateParameters = 10 };
    const char *templateParameters[maxTemplateParameters + 1];

    // internal state
    int extraInt[4];

    bool success;          // are we finished?
    bool full;
    int pos;
};


QDumper::QDumper()
{
    success = false;
    full = false;
    outBuffer[0] = 'f'; // marks output as 'wrong'
    pos = 1;
}

QDumper::~QDumper()
{
    outBuffer[pos++] = '\0';
    if (success)
        outBuffer[0] = (full ? '+' : 't');
}

void QDumper::setupTemplateParameters()
{
    char *s = const_cast<char *>(innertype);

    int templateParametersCount = 1;
    templateParameters[0] = s;
    for (int i = 1; i != maxTemplateParameters + 1; ++i)
        templateParameters[i] = 0;

    while (*s) {
        while (*s && *s != '@')
            ++s;
        if (*s) {
            *s = '\0';
            ++s;
            templateParameters[templateParametersCount++] = s;
        }
    }
    while (templateParametersCount < maxTemplateParameters)
        templateParameters[templateParametersCount++] = 0;
}

QDumper &QDumper::put(char c)
{
    checkFill();
    if (!full)
        outBuffer[pos++] = c;
    return *this;
}

QDumper &QDumper::put(unsigned long long c)
{
    checkFill();
    pos += sprintf(outBuffer + pos, "%llu", c);
    return *this;
}

QDumper &QDumper::put(long long c)
{
    checkFill();
    pos += sprintf(outBuffer + pos, "%lld", c);
    return *this;
}

QDumper &QDumper::put(unsigned long c)
{
    checkFill();
    pos += sprintf(outBuffer + pos, "%lu", c);
    return *this;
}

QDumper &QDumper::put(float d)
{
    checkFill();
    pos += sprintf(outBuffer + pos, "%f", d);
    return *this;
}

QDumper &QDumper::put(double d)
{
    checkFill();
    pos += sprintf(outBuffer + pos, "%f", d);
    return *this;
}

QDumper &QDumper::put(unsigned int i)
{
    checkFill();
    pos += sprintf(outBuffer + pos, "%u", i);
    return *this;
}

QDumper &QDumper::put(long c)
{
    checkFill();
    pos += sprintf(outBuffer + pos, "%ld", c);
    return *this;
}

QDumper &QDumper::put(int i)
{
    checkFill();
    pos += sprintf(outBuffer + pos, "%d", i);
    return *this;
}

QDumper &QDumper::put(const void *p)
{
    static char buf[100];
    if (p) {
        sprintf(buf, "%p", p);
        // we get a '0x' prefix only on some implementations.
        // if it isn't there, write it out manually.
        if (buf[1] != 'x') {
            put('0');
            put('x');
        }
        put(buf);
    } else {
        put("<null>");
    }
    return *this;
}

QDumper &QDumper::put(const char *str)
{
    if (!str)
        return put("<null>");
    while (*str)
        put(*(str++));
    return *this;
}

QDumper &QDumper::put(const QByteArray &ba)
{
    putBase64Encoded(ba.constData(), ba.size());
    return *this;
}

QDumper &QDumper::put(const QString &str)
{
    putBase64Encoded((const char *)str.constData(), 2 * str.size());
    return *this;
}

void QDumper::checkFill()
{
    if (pos >= int(sizeof(outBuffer)) - 100)
        full = true;
}

void QDumper::putCommaIfNeeded()
{
    if (pos == 0)
        return;
    char c = outBuffer[pos - 1];
    if (c == '}' || c == '"' || c == ']')
        put(',');
}

void QDumper::putBase64Encoded(const char *buf, int n)
{
    const char alphabet[] = "ABCDEFGH" "IJKLMNOP" "QRSTUVWX" "YZabcdef"
                            "ghijklmn" "opqrstuv" "wxyz0123" "456789+/";
    const char padchar = '=';
    int padlen = 0;

    //int tmpsize = ((n * 4) / 3) + 3;

    int i = 0;
    while (i < n) {
        int chunk = 0;
        chunk |= int(uchar(buf[i++])) << 16;
        if (i == n) {
            padlen = 2;
        } else {
            chunk |= int(uchar(buf[i++])) << 8;
            if (i == n)
                padlen = 1;
            else
                chunk |= int(uchar(buf[i++]));
        }

        int j = (chunk & 0x00fc0000) >> 18;
        int k = (chunk & 0x0003f000) >> 12;
        int l = (chunk & 0x00000fc0) >> 6;
        int m = (chunk & 0x0000003f);
        put(alphabet[j]);
        put(alphabet[k]);
        put(padlen > 1 ? padchar : alphabet[l]);
        put(padlen > 0 ? padchar : alphabet[m]);
    }
}

void QDumper::disarm()
{
    success = true;
}

void QDumper::beginHash()
{
    putCommaIfNeeded();
    put('{');
}

void QDumper::endHash()
{
    put('}');
}

void QDumper::putEllipsis()
{
    putCommaIfNeeded();
    put("{name=\"<incomplete>\",value=\"\",type=\"").put(innertype).put("\"}");
}

void QDumper::putItemCount(const char *name, int count)
{
    putCommaIfNeeded();
    put(name).put("=\"<").put(count).put(" items>\"");
}


//
// Some helpers to keep the dumper code short
//

void QDumper::beginItem(const char *name)
{
    putCommaIfNeeded();
    put(name).put('=').put('"');
}

void QDumper::endItem()
{
    put('"');
}

void QDumper::beginChildren()
{
    putCommaIfNeeded();
    put("children=[");
}

void QDumper::endChildren()
{
    put(']');
}

// simple string property
void QDumper::putHash(const char *name, const QString &value)
{
    beginHash();
    putItem("name", name);
    putItem("value", value);
    putItem("type", NS"QString");
    putItem("numchild", "0");
    putItem("valueencoded", "2");
    endHash();
}

void QDumper::putHash(const char *name, const QByteArray &value)
{
    beginHash();
    putItem("name", name);
    putItem("value", value);
    putItem("type", NS"QByteArray");
    putItem("numchild", "0");
    putItem("valueencoded", "1");
    endHash();
}

// simple integer property
void QDumper::putHash(const char *name, int value)
{
    beginHash();
    putItem("name", name);
    putItem("value", value);
    putItem("type", "int");
    putItem("numchild", "0");
    endHash();
}

void QDumper::putHash(const char *name, long value)
{
    beginHash();
    putItem("name", name);
    putItem("value", value);
    putItem("type", "long");
    putItem("numchild", "0");
    endHash();
}

// simple boolean property
void QDumper::putHash(const char *name, bool value)
{
    beginHash();
    putItem("name", name);
    putItem("value", (value ? "true" : "false"));
    putItem("type", "bool");
    putItem("numchild", "0");
    endHash();
}

// a single QChar
void QDumper::putHash(const char *name, QChar value)
{
    beginHash();
    putItem("name", name);
    putItem("value", QString(QLatin1String("'%1' (%2, 0x%3)"))
        .arg(value).arg(value.unicode()).arg(value.unicode(), 0, 16));
    putItem("valueencoded", "2");
    putItem("type", NS"QChar");
    putItem("numchild", "0");
    endHash();
}

#define DUMPUNKNOWN_MESSAGE "<internal error>"
static void qDumpUnknown(QDumper &d, const char *why = 0)
{
    //d.putItem("iname", d.iname);
    //d.putItem("addr", d.data);
    if (!why)
        why = DUMPUNKNOWN_MESSAGE;
    d.putItem("value", why);
    d.putItem("type", d.outertype);
    d.putItem("numchild", "0");
    d.disarm();
}

static inline void dumpStdStringValue(QDumper &d, const std::string &str)
{
    d.beginItem("value");
    d.putBase64Encoded(str.c_str(), str.size());
    d.endItem();
    d.putItem("valueencoded", "1");
    d.putItem("type", "std::string");
    d.putItem("numchild", "0");
}

static inline void dumpStdWStringValue(QDumper &d, const std::wstring &str)
{
    d.beginItem("value");
    d.putBase64Encoded((const char *)str.c_str(), str.size() * sizeof(wchar_t));
    d.endItem();
    d.putItem("valueencoded", (sizeof(wchar_t) == 2 ? "2" : "3"));
    d.putItem("type", "std::wstring");
    d.putItem("numchild", "0");
}

// Tell the calling routine whether a global "childnumchild" attribute makes sense
enum  InnerValueResult
{
    InnerValueNotHandled,
    InnerValueChildrenSpecified,
    InnerValueNoFurtherChildren,
    InnerValueFurtherChildren
};

static inline void dumpChildNumChildren(QDumper &d, InnerValueResult innerValueResult)
{
    switch (innerValueResult) {
    case InnerValueChildrenSpecified:
    case InnerValueNotHandled:
        break;
    case InnerValueNoFurtherChildren:
        d.putItem("childnumchild", "0");
        break;
    case InnerValueFurtherChildren:
        d.putItem("childnumchild", "1");
        break;
    }
}

// Called by templates, so, not static.
static void qDumpInnerQCharValue(QDumper &d, QChar c, const char *field)
{
    char buf[30];
    sprintf(buf, "'?', ucs=%d", c.unicode());
    if (c.isPrint() && c.unicode() < 127)
        buf[1] = char(c.unicode());
    d.putCommaIfNeeded();
    d.putItem(field, buf);
    d.putItem("numchild", 0);
}

static void qDumpInnerCharValue(QDumper &d, char c, const char *field)
{
    char buf[30];
    sprintf(buf, "'?', ascii=%d", c);
    if (QChar(c).isPrint() && c < 127)
        buf[1] = c;
    d.putCommaIfNeeded();
    d.putItem(field, buf);
    d.putItem("numchild", 0);
}

InnerValueResult qDumpInnerValueHelper(QDumper &d, const char *type, const void *addr,
    const char *field = "value")
{
    type = stripNamespace(type);
    switch (type[1]) {
        case 'h':
            if (isEqual(type, "char")) {
                qDumpInnerCharValue(d, *(char *)addr, field);
                return InnerValueNoFurtherChildren;
            }
            return InnerValueNotHandled;
        case 'l':
            if (isEqual(type, "float"))
                { d.putItem(field, *(float*)addr); return InnerValueNoFurtherChildren; }
            return InnerValueNotHandled;
        case 'n':
            if (isEqual(type, "int")) {
                d.putItem(field, *(int*)addr);
                return InnerValueNoFurtherChildren;
            }
            if (isEqual(type, "unsigned") || isEqual(type, "unsigned int")) {
                d.putItem(field, *(unsigned int*)addr);
                return InnerValueNoFurtherChildren;
            }
            if (isEqual(type, "unsigned char")) {
                qDumpInnerCharValue(d, *(char *)addr, field);
                return InnerValueNoFurtherChildren;
            }
            if (isEqual(type, "unsigned long")) {
                d.putItem(field, *(unsigned long*)addr);
                return InnerValueNoFurtherChildren;
            }
            if (isEqual(type, "unsigned long long")) {
                d.putItem(field, *(qulonglong*)addr);
                return InnerValueNoFurtherChildren;
            }
            return InnerValueNotHandled;
        case 'o':
            if (isEqual(type, "bool")) {
                switch (*(bool*)addr) {
                case 0: d.putItem(field, "false"); break;
                case 1: d.putItem(field, "true"); break;
                default: d.putItem(field, *(bool*)addr); break;
                }
                return InnerValueNoFurtherChildren;
            }
            if (isEqual(type, "double")) {
                d.putItem(field, *(double*)addr);
                return InnerValueNoFurtherChildren;
            }
            if (isEqual(type, "long")) {
                d.putItem(field, *(long*)addr);
                return InnerValueNoFurtherChildren;
            }
            else if (isEqual(type, "long long")) {
                d.putItem(field, *(qulonglong*)addr);
                return InnerValueNoFurtherChildren;
            }
            return InnerValueNotHandled;
        case 'B':
            if (isEqual(type, "QByteArray")) {
                d.putCommaIfNeeded();
                d.put(field).put("encoded=\"1\",");
                d.putItem(field, *(QByteArray*)addr);
                return InnerValueFurtherChildren;
            }
            return InnerValueNotHandled;
        case 'C':
            if (isEqual(type, "QChar")) {
                qDumpInnerQCharValue(d, *(QChar*)addr, field);
                return InnerValueNoFurtherChildren;
            }
            return InnerValueNotHandled;
        case 'L':
            if (startsWith(type, "QList<")) {
                const QListData *ldata = reinterpret_cast<const QListData*>(addr);
                d.putItemCount("value", ldata->size());
                d.putItem("valuedisabled", "true");
                d.putItem("numchild", ldata->size());
                return InnerValueChildrenSpecified;
            }
            return InnerValueNotHandled;
        case 'O':
            if (isEqual(type, "QObject *")) {
                if (addr) {
                    const QObject *ob = reinterpret_cast<const QObject *>(addr);
                    d.putItem("addr", ob);
                    d.putItem("value", ob->objectName());
                    d.putItem("valueencoded", "2");
                    d.putItem("type", NS"QObject");
                    d.putItem("displayedtype", ob->metaObject()->className());
                    d.putItem("numchild", 1);
                    return InnerValueChildrenSpecified;
                } else {
                    d.putItem("value", "0x0");
                    d.putItem("type", NS"QObject *");
                    d.putItem("numchild", 0);
                    return InnerValueNoFurtherChildren;
                }
            }
            return InnerValueNotHandled;
        case 'S':
            if (isEqual(type, "QString")) {
                d.putCommaIfNeeded();
                d.putItem(field, *(QString*)addr);
                d.put(',').put(field).put("encoded=\"2\"");
                return InnerValueNoFurtherChildren;
            }
            return InnerValueNotHandled;
        case 't':
            if (isEqual(type, "std::string")
                || isEqual(type, stdStringTypeC)) {
                d.putCommaIfNeeded();
                dumpStdStringValue(d, *reinterpret_cast<const std::string*>(addr));
                return InnerValueNoFurtherChildren;
            }
            if (isEqual(type, "std::wstring")
                || isEqual(type, stdWideStringTypeUShortC)) {
                dumpStdWStringValue(d, *reinterpret_cast<const std::wstring*>(addr));
                return InnerValueNoFurtherChildren;
            }
            return InnerValueNotHandled;
        default:
            break;
    }
    return InnerValueNotHandled;
}

static InnerValueResult qDumpInnerValue(QDumper &d, const char *type, const void *addr)
{
    d.putItem("addr", addr);
    d.putItem("type", type);

    if (!type[0])
        return InnerValueNotHandled;

    return qDumpInnerValueHelper(d, type, addr);
}

static InnerValueResult qDumpInnerValueOrPointer(QDumper &d,
    const char *type, const char *strippedtype, const void *addr)
{
    if (strippedtype) {
        if (deref(addr)) {
            d.putItem("addr", deref(addr));
            d.putItem("saddr", deref(addr));
            d.putItem("type", strippedtype);
            return qDumpInnerValueHelper(d, strippedtype, deref(addr));
        }
        d.putItem("addr", addr);
        d.putItem("type", strippedtype);
        d.putItem("value", "<null>");
        d.putItem("numchild", "0");
        return InnerValueChildrenSpecified;
    }
    d.putItem("addr", addr);
    d.putItem("type", type);
    return qDumpInnerValueHelper(d, type, addr);
}

//////////////////////////////////////////////////////////////////////////////

struct ModelIndex { int r; int c; void *p; void *m; };

static void qDumpQAbstractItem(QDumper &d)
{
    ModelIndex mm;
    mm.r = mm.c = 0;
    mm.p = mm.m = 0;
    sscanf(d.templateParameters[0], "%d,%d,%p,%p", &mm.r, &mm.c, &mm.p, &mm.m);
    const QModelIndex &mi(*reinterpret_cast<QModelIndex *>(&mm));
    const QAbstractItemModel *m = mi.model();
    const int rowCount = m->rowCount(mi);
    if (rowCount < 0)
        return;
    const int columnCount = m->columnCount(mi);
    if (columnCount < 0)
        return;
    d.putItem("type", NS"QAbstractItem");
    d.beginItem("addr");
        d.put('$').put(mm.r).put(',').put(mm.c).put(',').put(mm.p).put(',').put(mm.m);
    d.endItem();
    //d.putItem("value", "(").put(rowCount).put(",").put(columnCount).put(")");
    d.putItem("value", m->data(mi, Qt::DisplayRole).toString());
    d.putItem("valueencoded", "2");
    d.putItem("numchild", "1");
    if (d.dumpChildren) {
        d.beginChildren();
        for (int row = 0; row < rowCount; ++row) {
            for (int column = 0; column < columnCount; ++column) {
                QModelIndex child = m->index(row, column, mi);
                d.beginHash();
                d.beginItem("name");
                    d.put("[").put(row).put(",").put(column).put("]");
                d.endItem();
                //d.putItem("numchild", (m->hasChildren(child) ? "1" : "0"));
                d.putItem("numchild", "1");
                d.beginItem("addr");
                    d.put("$").put(child.row()).put(",").put(child.column()).put(",")
                        .put(child.internalPointer()).put(",").put(child.model());
                d.endItem();
                d.putItem("type", NS"QAbstractItem");
                d.putItem("value", m->data(mi, Qt::DisplayRole).toString());
                d.putItem("valueencoded", "2");
                d.endHash();
            }
        }
/*
        d.beginHash();
        d.putItem("name", "DisplayRole");
        d.putItem("numchild", 0);
        d.putItem("value", m->data(mi, Qt::DisplayRole).toString());
        d.putItem("valueencoded", 2);
        d.putItem("type", NS"QString");
        d.endHash();
*/
        d.endChildren();
    }
    d.disarm();
}

static void qDumpQAbstractItemModel(QDumper &d)
{
    const QAbstractItemModel &m = *reinterpret_cast<const QAbstractItemModel *>(d.data);

    const int rowCount = m.rowCount();
    if (rowCount < 0)
        return;
    const int columnCount = m.columnCount();
    if (columnCount < 0)
        return;

    d.putItem("type", NS"QAbstractItemModel");
    d.beginItem("value");
        d.put("(").put(rowCount).put(",").put(columnCount).put(")");
    d.endItem();
    d.putItem("numchild", "1");
    if (d.dumpChildren) {
        d.beginChildren();
        d.beginHash();
            d.putItem("numchild", "1");
            d.putItem("name", NS"QObject");
            d.putItem("addr", d.data);
            d.putItem("value", m.objectName());
            d.putItem("valueencoded", "2");
            d.putItem("type", NS"QObject");
            d.putItem("displayedtype", m.metaObject()->className());
        d.endHash();
        for (int row = 0; row < rowCount; ++row) {
            for (int column = 0; column < columnCount; ++column) {
                QModelIndex mi = m.index(row, column);
                d.beginHash();
                d.beginItem("name");
                    d.put("[").put(row).put(",").put(column).put("]");
                d.endItem();
                d.putItem("value", m.data(mi, Qt::DisplayRole).toString());
                d.putItem("valueencoded", "2");
                //d.putItem("numchild", (m.hasChildren(mi) ? "1" : "0"));
                d.putItem("numchild", "1");
                d.beginItem("addr");
                    d.put("$").put(mi.row()).put(",").put(mi.column()).put(",");
                    d.put(mi.internalPointer()).put(",").put(mi.model());
                d.endItem();
                d.putItem("type", NS"QAbstractItem");
                d.endHash();
            }
        }
        d.endChildren();
    }
    d.disarm();
}

static void qDumpQByteArray(QDumper &d)
{
    const QByteArray &ba = *reinterpret_cast<const QByteArray *>(d.data);

    if (!ba.isEmpty()) {
        qCheckAccess(ba.constData());
        qCheckAccess(ba.constData() + ba.size());
    }

    d.beginItem("value");
    if (ba.size() <= 100)
        d.put(ba);
    else
        d.put(ba.left(100)).put(" <size: ").put(ba.size()).put(", cut...>");
    d.endItem();
    d.putItem("valueencoded", "1");
    d.putItem("type", NS"QByteArray");
    d.putItem("numchild", ba.size());
    if (d.dumpChildren) {
        d.putItem("childtype", "char");
        d.putItem("childnumchild", "0");
        d.beginChildren();
        char buf[20];
        for (int i = 0; i != ba.size(); ++i) {
            unsigned char c = ba.at(i);
            unsigned char u = (isprint(c) && c != '\'' && c != '"') ? c : '?';
            sprintf(buf, "%02x  (%u '%c')", c, c, u);
            d.beginHash();
            d.putItem("name", i);
            d.putItem("value", buf);
            d.endHash();
        }
        d.endChildren();
    }
    d.disarm();
}

static void qDumpQChar(QDumper &d)
{
    qDumpInnerQCharValue(d, *reinterpret_cast<const QChar *>(d.data), "value");
    d.disarm();
}

static void qDumpQDateTime(QDumper &d)
{
#ifdef QT_NO_DATESTRING
    qDumpUnknown(d);
#else
    const QDateTime &date = *reinterpret_cast<const QDateTime *>(d.data);
    if (date.isNull()) {
        d.putItem("value", "(null)");
    } else {
        d.putItem("value", date.toString());
        d.putItem("valueencoded", "2");
    }
    d.putItem("type", NS"QDateTime");
    d.putItem("numchild", "3");
    if (d.dumpChildren) {
        d.beginChildren();
        d.putHash("isNull", date.isNull());
        d.putHash("toTime_t", (long)date.toTime_t());
        d.putHash("toString", date.toString());
        #if QT_VERSION >= 0x040500
        d.putHash("toString_(ISO)", date.toString(Qt::ISODate));
        d.putHash("toString_(SystemLocale)", date.toString(Qt::SystemLocaleDate));
        d.putHash("toString_(Locale)", date.toString(Qt::LocaleDate));
        #endif

        #if 0
        d.beginHash();
        d.putItem("name", "toUTC");
        d.putItem("exp", "(("NSX"QDateTime"NSY"*)").put(d.data).put(")"
                    "->toTimeSpec('"NS"Qt::UTC')");
        d.putItem("type", NS"QDateTime");
        d.putItem("numchild", "1");
        d.endHash();
        #endif

        #if 0
        d.beginHash();
        d.putItem("name", "toLocalTime");
        d.putItem("exp", "(("NSX"QDateTime"NSY"*)").put(d.data).put(")"
                    "->toTimeSpec('"NS"Qt::LocalTime')");
        d.putItem("type", NS"QDateTime");
        d.putItem("numchild", "1");
        d.endHash();
        #endif

        d.endChildren();
    }
    d.disarm();
#endif // ifdef QT_NO_DATESTRING
}

static void qDumpQDir(QDumper &d)
{
    const QDir &dir = *reinterpret_cast<const QDir *>(d.data);
    d.putItem("value", dir.path());
    d.putItem("valueencoded", "2");
    d.putItem("type", NS"QDir");
    d.putItem("numchild", "3");
    if (d.dumpChildren) {
        d.beginChildren();
        d.putHash("absolutePath", dir.absolutePath());
        d.putHash("canonicalPath", dir.canonicalPath());
        d.endChildren();
    }
    d.disarm();
}

static void qDumpQFile(QDumper &d)
{
    const QFile &file = *reinterpret_cast<const QFile *>(d.data);
    d.putItem("value", file.fileName());
    d.putItem("valueencoded", "2");
    d.putItem("type", NS"QFile");
    d.putItem("numchild", "2");
    if (d.dumpChildren) {
        d.beginChildren();
        d.putHash("fileName", file.fileName());
        d.putHash("exists", file.exists());
        d.endChildren();
    }
    d.disarm();
}

static void qDumpQFileInfo(QDumper &d)
{
    const QFileInfo &info = *reinterpret_cast<const QFileInfo *>(d.data);
    d.putItem("value", info.filePath());
    d.putItem("valueencoded", "2");
    d.putItem("type", NS"QFileInfo");
    d.putItem("numchild", "3");
    if (d.dumpChildren) {
        d.beginChildren();
        d.putHash("absolutePath", info.absolutePath());
        d.putHash("absoluteFilePath", info.absoluteFilePath());
        d.putHash("canonicalPath", info.canonicalPath());
        d.putHash("canonicalFilePath", info.canonicalFilePath());
        d.putHash("completeBaseName", info.completeBaseName());
        d.putHash("completeSuffix", info.completeSuffix());
        d.putHash("baseName", info.baseName());
#ifdef Q_OS_MACX
        d.putHash("isBundle", info.isBundle());
        d.putHash("bundleName", info.bundleName());
#endif
        d.putHash("completeSuffix", info.completeSuffix());
        d.putHash("fileName", info.fileName());
        d.putHash("filePath", info.filePath());
        d.putHash("group", info.group());
        d.putHash("owner", info.owner());
        d.putHash("path", info.path());

        d.putHash("groupid", (long)info.groupId());
        d.putHash("ownerid", (long)info.ownerId());
        //QFile::Permissions permissions () const
        d.putHash("permissions", (long)info.permissions());

        //QDir absoluteDir () const
        //QDir dir () const

        d.putHash("caching", info.caching());
        d.putHash("exists", info.exists());
        d.putHash("isAbsolute", info.isAbsolute());
        d.putHash("isDir", info.isDir());
        d.putHash("isExecutable", info.isExecutable());
        d.putHash("isFile", info.isFile());
        d.putHash("isHidden", info.isHidden());
        d.putHash("isReadable", info.isReadable());
        d.putHash("isRelative", info.isRelative());
        d.putHash("isRoot", info.isRoot());
        d.putHash("isSymLink", info.isSymLink());
        d.putHash("isWritable", info.isWritable());

        d.beginHash();
        d.putItem("name", "created");
        d.putItem("value", info.created().toString());
        d.putItem("valueencoded", "2");
        d.beginItem("exp");
            d.put("(("NSX"QFileInfo"NSY"*)").put(d.data).put(")->created()");
        d.endItem();
        d.putItem("type", NS"QDateTime");
        d.putItem("numchild", "1");
        d.endHash();

        d.beginHash();
        d.putItem("name", "lastModified");
        d.putItem("value", info.lastModified().toString());
        d.putItem("valueencoded", "2");
        d.beginItem("exp");
            d.put("(("NSX"QFileInfo"NSY"*)").put(d.data).put(")->lastModified()");
        d.endItem();
        d.putItem("type", NS"QDateTime");
        d.putItem("numchild", "1");
        d.endHash();

        d.beginHash();
        d.putItem("name", "lastRead");
        d.putItem("value", info.lastRead().toString());
        d.putItem("valueencoded", "2");
        d.beginItem("exp");
            d.put("(("NSX"QFileInfo"NSY"*)").put(d.data).put(")->lastRead()");
        d.endItem();
        d.putItem("type", NS"QDateTime");
        d.putItem("numchild", "1");
        d.endHash();

        d.endChildren();
    }
    d.disarm();
}

bool isOptimizedIntKey(const char *keyType)
{
    return isEqual(keyType, "int")
#if defined(Q_BYTE_ORDER) && Q_BYTE_ORDER == Q_LITTLE_ENDIAN
        || isEqual(keyType, "short")
        || isEqual(keyType, "ushort")
#endif
        || isEqual(keyType, "uint");
}

int hashOffset(bool optimizedIntKey, bool forKey, unsigned keySize, unsigned valueSize)
{
    // int-key optimization, small value
    struct NodeOS { void *next; uint k; uint  v; } nodeOS;
    // int-key optimiatzion, large value
    struct NodeOL { void *next; uint k; void *v; } nodeOL;
    // no optimization, small value
    struct NodeNS { void *next; uint h; uint  k; uint  v; } nodeNS;
    // no optimization, large value
    struct NodeNL { void *next; uint h; uint  k; void *v; } nodeNL;
    // complex key
    struct NodeL  { void *next; uint h; void *k; void *v; } nodeL;

    if (forKey) {
        // offsetof(...,...) not yet in Standard C++
        const ulong nodeOSk ( (char *)&nodeOS.k - (char *)&nodeOS );
        const ulong nodeOLk ( (char *)&nodeOL.k - (char *)&nodeOL );
        const ulong nodeNSk ( (char *)&nodeNS.k - (char *)&nodeNS );
        const ulong nodeNLk ( (char *)&nodeNL.k - (char *)&nodeNL );
        const ulong nodeLk  ( (char *)&nodeL.k  - (char *)&nodeL );
        if (optimizedIntKey)
            return valueSize > sizeof(int) ? nodeOLk : nodeOSk;
        if (keySize > sizeof(int))
            return nodeLk;
        return valueSize > sizeof(int) ? nodeNLk : nodeNSk;
    } else {
        const ulong nodeOSv ( (char *)&nodeOS.v - (char *)&nodeOS );
        const ulong nodeOLv ( (char *)&nodeOL.v - (char *)&nodeOL );
        const ulong nodeNSv ( (char *)&nodeNS.v - (char *)&nodeNS );
        const ulong nodeNLv ( (char *)&nodeNL.v - (char *)&nodeNL );
        const ulong nodeLv  ( (char *)&nodeL.v  - (char *)&nodeL );
        if (optimizedIntKey)
            return valueSize > sizeof(int) ? nodeOLv : nodeOSv;
        if (keySize > sizeof(int))
            return nodeLv;
        return valueSize > sizeof(int) ? nodeNLv : nodeNSv;
    }
}

#ifdef Q_CC_MSVC
#  define MAP_NODE_TYPE_END ">"
#else
#  define MAP_NODE_TYPE_END " >"
#endif

static void qDumpQHash(QDumper &d)
{
    QHashData *h = *reinterpret_cast<QHashData *const*>(d.data);
    const char *keyType   = d.templateParameters[0];
    const char *valueType = d.templateParameters[1];

    qCheckPointer(h->fakeNext);
    qCheckPointer(h->buckets);

    unsigned keySize = d.extraInt[0];
    unsigned valueSize = d.extraInt[1];

    int n = h->size;

    if (n < 0)
        return;
    if (n > 0) {
        qCheckPointer(h->fakeNext);
        qCheckPointer(*h->buckets);
    }

    d.putItemCount("value", n);
    d.putItem("numchild", n);

    if (d.dumpChildren) {
        if (n > 1000)
            n = 1000;
        const bool isSimpleKey = isSimpleType(keyType);
        const bool isSimpleValue = isSimpleType(valueType);
        const bool opt = isOptimizedIntKey(keyType);
        const int keyOffset = hashOffset(opt, true, keySize, valueSize);
        const int valueOffset = hashOffset(opt, false, keySize, valueSize);

        d.beginItem("extra");
        d.put("isSimpleKey: ").put(isSimpleKey);
        d.put(" isSimpleValue: ").put(isSimpleValue);
        d.put(" valueType: '").put(isSimpleValue);
        d.put(" keySize: ").put(keyOffset);
        d.put(" valueOffset: ").put(valueOffset);
        d.put(" opt: ").put(opt);
        d.endItem();

        QHashData::Node *node = h->firstNode();
        QHashData::Node *end = reinterpret_cast<QHashData::Node *>(h);
        int i = 0;

        d.beginChildren();
        while (node != end) {
            d.beginHash();
                d.putItem("name", i);
                qDumpInnerValueHelper(d, keyType, addOffset(node, keyOffset), "key");
                qDumpInnerValueHelper(d, valueType, addOffset(node, valueOffset));
                if (isSimpleKey && isSimpleValue) {
                    d.putItem("type", valueType);
                    d.putItem("addr", addOffset(node, valueOffset));
                } else {
                    d.putItem("addr", node);
                    d.beginItem("type");
                        d.put("'"NS"QHashNode<").put(keyType).put(",")
                            .put(valueType).put(MAP_NODE_TYPE_END"'");
                    d.endItem();
                }
            d.endHash();
            ++i;
            node = QHashData::nextNode(node);
        }
        d.endChildren();
    }
    d.disarm();
}

static void qDumpQHashNode(QDumper &d)
{
    const QHashData *h = reinterpret_cast<const QHashData *>(d.data);
    const char *keyType   = d.templateParameters[0];
    const char *valueType = d.templateParameters[1];

    unsigned keySize = d.extraInt[0];
    unsigned valueSize = d.extraInt[1];
    bool opt = isOptimizedIntKey(keyType);
    int keyOffset = hashOffset(opt, true, keySize, valueSize);
    int valueOffset = hashOffset(opt, false, keySize, valueSize);
    if (isSimpleType(valueType))
        qDumpInnerValueHelper(d, valueType, addOffset(h, valueOffset));
    else
        d.putItem("value", "");

    d.putItem("numchild", 2);
    if (d.dumpChildren) {
        // there is a hash specialization in case the keys are integers or shorts
        d.beginChildren();
        d.beginHash();
            d.putItem("name", "key");
            d.putItem("type", keyType);
            d.putItem("addr", addOffset(h, keyOffset));
        d.endHash();
        d.beginHash();
            d.putItem("name", "value");
            d.putItem("type", valueType);
            d.putItem("addr", addOffset(h, valueOffset));
        d.endHash();
        d.endChildren();
    }
    d.disarm();
}

#if USE_QT_GUI
static void qDumpQImage(QDumper &d)
{
    const QImage &im = *reinterpret_cast<const QImage *>(d.data);
    d.beginItem("value");
        d.put("(").put(im.width()).put("x").put(im.height()).put(")");
    d.endItem();
    d.putItem("type", NS"QImage");
    d.putItem("numchild", "1");
    if (d.dumpChildren) {
        d.beginChildren();
        d.beginHash();
            d.putItem("name", "data");
            d.putItem("type", NS "QImageData");
            d.putItem("addr", d.data);
        d.endHash();
    }
    d.disarm();
}
#endif

#if USE_QT_GUI
static void qDumpQImageData(QDumper &d)
{
    const QImage &im = *reinterpret_cast<const QImage *>(d.data);
    const QByteArray ba(QByteArray::fromRawData((const char*)im.bits(), im.numBytes()));
    d.putItem("type", NS"QImageData");
    d.putItem("numchild", "0");
#if 1
    d.putItem("value", "<hover here>");
    d.putItem("valuetooltipencoded", "1");
    d.putItem("valuetooltipsize", ba.size());
    d.putItem("valuetooltip", ba);
#else
    d.putItem("valueencoded", "1");
    d.putItem("value", ba);
#endif
    d.disarm();
}
#endif

static void qDumpQList(QDumper &d)
{
    // This uses the knowledge that QList<T> has only a single member
    // of type  union { QListData p; QListData::Data *d; };

    const QListData &ldata = *reinterpret_cast<const QListData*>(d.data);
    const QListData::Data *pdata =
        *reinterpret_cast<const QListData::Data* const*>(d.data);
    qCheckAccess(pdata);
    int nn = ldata.size();
    if (nn < 0)
        return;
    if (nn > 0) {
        qCheckAccess(ldata.d->array);
        //qCheckAccess(ldata.d->array[0]);
        //qCheckAccess(ldata.d->array[nn - 1]);
#if QT_VERSION >= 0x040400
        if (ldata.d->ref._q_value <= 0)
            return;
#endif
    }

    int n = nn;
    d.putItemCount("value", n);
    d.putItem("valuedisabled", "true");
    d.putItem("numchild", n);
    if (d.dumpChildren) {
        InnerValueResult innerValueResult = InnerValueChildrenSpecified;
        unsigned innerSize = d.extraInt[0];
        bool innerTypeIsPointer = isPointerType(d.innertype);
        QByteArray strippedInnerType = stripPointerType(d.innertype);

        // The exact condition here is:
        //  QTypeInfo<T>::isLarge || QTypeInfo<T>::isStatic
        // but this data is available neither in the compiled binary nor
        // in the frontend.
        // So as first approximation only do the 'isLarge' check:
        bool isInternal = innerSize <= int(sizeof(void*))
            && isMovableType(d.innertype);

        d.putItem("internal", (int)isInternal);
        d.putItem("childtype", d.innertype);
        if (n > 1000)
            n = 1000;
        d.beginChildren();
        for (int i = 0; i != n; ++i) {
            d.beginHash();
            d.putItem("name", i);
            if (innerTypeIsPointer) {
                void *p = ldata.d->array + i + pdata->begin;
                d.putItem("saddr", p);
                if (*(void**)p) {
                    //d.putItem("value","@").put(p);
                    innerValueResult = qDumpInnerValue(d, strippedInnerType.data(), deref(p));
                } else {
                    d.putItem("value", "<null>");
                    d.putItem("numchild", "0");
                }
            } else {
                void *p = ldata.d->array + i + pdata->begin;
                if (isInternal) {
                    //qDumpInnerValue(d, d.innertype, p);
                    d.putItem("addr", p);
                    innerValueResult = qDumpInnerValueHelper(d, d.innertype, p);
                } else {
                    //qDumpInnerValue(d, d.innertype, deref(p));
                    d.putItem("addr", deref(p));
                    innerValueResult = qDumpInnerValueHelper(d, d.innertype, deref(p));
                }
            }
            d.endHash();
        }
        if (n < nn)
            d.putEllipsis();
        d.endChildren();
        dumpChildNumChildren(d, innerValueResult);
    }
    d.disarm();
}

static void qDumpQLinkedList(QDumper &d)
{
    // This uses the knowledge that QLinkedList<T> has only a single member
    // of type  union { QLinkedListData *d; QLinkedListNode<T> *e; };
    const QLinkedListData *ldata =
        reinterpret_cast<const QLinkedListData*>(deref(d.data));
    int nn = ldata->size;
    if (nn < 0)
        return;

    int n = nn;
    d.putItemCount("value", n);
    d.putItem("valuedisabled", "true");
    d.putItem("numchild", n);
    InnerValueResult innerValueResult = InnerValueChildrenSpecified;
    if (d.dumpChildren) {
        //unsigned innerSize = d.extraInt[0];
        //bool innerTypeIsPointer = isPointerType(d.innertype);
        QByteArray strippedInnerType = stripPointerType(d.innertype);
        const char *stripped =
            isPointerType(d.innertype) ? strippedInnerType.data() : 0;

        d.putItem("childtype", d.innertype);
        if (n > 1000)
            n = 1000;
        d.beginChildren();
        const void *p = deref(ldata);
        for (int i = 0; i != n; ++i) {
            d.beginHash();
            d.putItem("name", i);
            const void *addr = addOffset(p, 2 * sizeof(void*));
            innerValueResult = qDumpInnerValueOrPointer(d, d.innertype, stripped, addr);
            p = deref(p);
            d.endHash();
        }
        if (n < nn)
            d.putEllipsis();
        d.endChildren();
    }
    dumpChildNumChildren(d, innerValueResult);
    d.disarm();
}

static void qDumpQLocale(QDumper &d)
{
    const QLocale &locale = *reinterpret_cast<const QLocale *>(d.data);
    d.putItem("value", locale.name());
    d.putItem("valueencoded", "2");
    d.putItem("type", NS"QLocale");
    d.putItem("numchild", "8");
    if (d.dumpChildren) {
        d.beginChildren();

        d.beginHash();
        d.putItem("name", "country");
        d.beginItem("exp");
        d.put("(("NSX"QLocale"NSY"*)").put(d.data).put(")->country()");
        d.endItem();
        d.endHash();

        d.beginHash();
        d.putItem("name", "language");
        d.beginItem("exp");
        d.put("(("NSX"QLocale"NSY"*)").put(d.data).put(")->language()");
        d.endItem();
        d.endHash();

        d.beginHash();
        d.putItem("name", "measurementSystem");
        d.beginItem("exp");
        d.put("(("NSX"QLocale"NSY"*)").put(d.data).put(")->measurementSystem()");
        d.endItem();
        d.endHash();

        d.beginHash();
        d.putItem("name", "numberOptions");
        d.beginItem("exp");
        d.put("(("NSX"QLocale"NSY"*)").put(d.data).put(")->numberOptions()");
        d.endItem();
        d.endHash();

        d.putHash("timeFormat_(short)", locale.timeFormat(QLocale::ShortFormat));
        d.putHash("timeFormat_(long)", locale.timeFormat(QLocale::LongFormat));

        d.putHash("decimalPoint", locale.decimalPoint());
        d.putHash("exponential", locale.exponential());
        d.putHash("percent", locale.percent());
        d.putHash("zeroDigit", locale.zeroDigit());
        d.putHash("groupSeparator", locale.groupSeparator());
        d.putHash("negativeSign", locale.negativeSign());

        d.endChildren();
    }
    d.disarm();
}

static void qDumpQMapNode(QDumper &d)
{
    const QMapData *h = reinterpret_cast<const QMapData *>(d.data);
    const char *keyType   = d.templateParameters[0];
    const char *valueType = d.templateParameters[1];

    qCheckAccess(h->backward);
    qCheckAccess(h->forward[0]);

    d.putItem("value", "");
    d.putItem("numchild", 2);
    InnerValueResult innerValueResult = InnerValueChildrenSpecified;
    if (d.dumpChildren) {
        unsigned mapnodesize = d.extraInt[2];
        unsigned valueOff = d.extraInt[3];

        unsigned keyOffset = 2 * sizeof(void*) - mapnodesize;
        unsigned valueOffset = 2 * sizeof(void*) - mapnodesize + valueOff;

        d.beginChildren();
        d.beginHash();
        d.putItem("name", "key");
        qDumpInnerValue(d, keyType, addOffset(h, keyOffset));

        d.endHash();
        d.beginHash();
        d.putItem("name", "value");
        innerValueResult = qDumpInnerValue(d, valueType, addOffset(h, valueOffset));
        d.endHash();
        d.endChildren();
    }
    dumpChildNumChildren(d, innerValueResult);
    d.disarm();
}

static void qDumpQMap(QDumper &d)
{
    QMapData *h = *reinterpret_cast<QMapData *const*>(d.data);
    const char *keyType   = d.templateParameters[0];
    const char *valueType = d.templateParameters[1];

    int n = h->size;

    if (n < 0)
        return;
    if (n > 0) {
        qCheckAccess(h->backward);
        qCheckAccess(h->forward[0]);
        qCheckPointer(h->backward->backward);
        qCheckPointer(h->forward[0]->backward);
    }

    d.putItemCount("value", n);
    d.putItem("numchild", n);
    if (d.dumpChildren) {
        if (n > 1000)
            n = 1000;

        //unsigned keySize = d.extraInt[0];
        //unsigned valueSize = d.extraInt[1];
        unsigned mapnodesize = d.extraInt[2];
        unsigned valueOff = d.extraInt[3];

        bool isSimpleKey = isSimpleType(keyType);
        bool isSimpleValue = isSimpleType(valueType);
        // both negative:
        int keyOffset = 2 * sizeof(void*) - int(mapnodesize);
        int valueOffset = 2 * sizeof(void*) - int(mapnodesize) + valueOff;

        d.beginItem("extra");
        d.put("simplekey: ").put(isSimpleKey).put(" isSimpleValue: ").put(isSimpleValue);
        d.put(" keyOffset: ").put(keyOffset).put(" valueOffset: ").put(valueOffset);
        d.put(" mapnodesize: ").put(mapnodesize);
        d.endItem();
        d.beginChildren();

        QMapData::Node *node = reinterpret_cast<QMapData::Node *>(h->forward[0]);
        QMapData::Node *end = reinterpret_cast<QMapData::Node *>(h);
        int i = 0;

        while (node != end) {
            d.beginHash();
                d.putItem("name", i);
                qDumpInnerValueHelper(d, keyType, addOffset(node, keyOffset), "key");
                qDumpInnerValueHelper(d, valueType, addOffset(node, valueOffset));
                if (isSimpleKey && isSimpleValue) {
                    d.putItem("type", valueType);
                    d.putItem("addr", addOffset(node, valueOffset));
                } else {
#if QT_VERSION >= 0x040500
                    d.putItem("addr", node);
                    // actually, any type (even 'char') will do...
                    d.beginItem("type");
                        d.put(NS"QMapNode<").put(keyType).put(",");
                        d.put(valueType).put(MAP_NODE_TYPE_END);
                    d.endItem();
#else
                    d.beginItem("type");
                        d.put(NS"QMapData::Node<").put(keyType).put(",");
                        d.put(valueType).put(MAP_NODE_TYPE_END);
                    d.endItem();
                    d.beginItem("exp");
                        d.put("*('"NS"QMapData::Node<").put(keyType).put(",");
                        d.put(valueType).put(" >'*)").put(node);
                    d.endItem();
#endif
                }
            d.endHash();

            ++i;
            node = node->forward[0];
        }
        d.endChildren();
    }

    d.disarm();
}

static void qDumpQMultiMap(QDumper &d)
{
    qDumpQMap(d);
}

static void qDumpQModelIndex(QDumper &d)
{
    const QModelIndex *mi = reinterpret_cast<const QModelIndex *>(d.data);

    d.putItem("type", NS"QModelIndex");
    if (mi->isValid()) {
        d.beginItem("value");
            d.put("(").put(mi->row()).put(", ").put(mi->column()).put(")");
        d.endItem();
        d.putItem("numchild", 5);
        if (d.dumpChildren) {
            d.beginChildren();
            d.putHash("row", mi->row());
            d.putHash("column", mi->column());

            d.beginHash();
            d.putItem("name", "parent");
            const QModelIndex parent = mi->parent();
            d.beginItem("value");
            if (parent.isValid())
                d.put("(").put(mi->row()).put(", ").put(mi->column()).put(")");
            else
                d.put("<invalid>");
            d.endItem();
            d.beginItem("exp");
                d.put("(("NSX"QModelIndex"NSY"*)").put(d.data).put(")->parent()");
            d.endItem();
            d.putItem("type", NS"QModelIndex");
            d.putItem("numchild", "1");
            d.endHash();

            d.putHash("internalId", QString::number(mi->internalId(), 10));

            d.beginHash();
            d.putItem("name", "model");
            d.putItem("value", static_cast<const void *>(mi->model()));
            d.putItem("type", NS"QAbstractItemModel*");
            d.putItem("numchild", "1");
            d.endHash();

            d.endChildren();
        }
    } else {
        d.putItem("value", "<invalid>");
        d.putItem("numchild", 0);
    }

    d.disarm();
}

static void qDumpQObject(QDumper &d)
{
    const QObject *ob = reinterpret_cast<const QObject *>(d.data);
    const QMetaObject *mo = ob->metaObject();
    d.putItem("value", ob->objectName());
    d.putItem("valueencoded", "2");
    d.putItem("type", NS"QObject");
    d.putItem("displayedtype", mo->className());
    d.putItem("numchild", 4);
    if (d.dumpChildren) {
        int slotCount = 0;
        int signalCount = 0;
        for (int i = mo->methodCount(); --i >= 0; ) {
            QMetaMethod::MethodType mt = mo->method(i).methodType();
            signalCount += (mt == QMetaMethod::Signal);
            slotCount += (mt == QMetaMethod::Slot);
        }
        d.beginChildren();
        d.beginHash();
            d.putItem("name", "properties");
            // using 'addr' does not work in gdb as 'exp' is recreated as
            // (type *)addr, and here we have different 'types':
            // QObject vs QObjectPropertyList!
            d.putItem("addr", d.data);
            d.putItem("type", NS"QObjectPropertyList");
            d.putItemCount("value", mo->propertyCount());
            d.putItem("numchild", mo->propertyCount());
        d.endHash();
        d.beginHash();
            d.putItem("name", "signals");
            d.putItem("addr", d.data);
            d.putItem("type", NS"QObjectSignalList");
            d.putItemCount("value", signalCount);
            d.putItem("numchild", signalCount);
        d.endHash();
        d.beginHash();
            d.putItem("name", "slots");
            d.putItem("addr", d.data);
            d.putItem("type", NS"QObjectSlotList");
            d.putItemCount("value", slotCount);
            d.putItem("numchild", slotCount);
        d.endHash();
        const QObjectList objectChildren = ob->children();
        if (!objectChildren.empty()) {
            d.beginHash();
            d.putItem("name", "children");
            d.putItem("addr", d.data);
            d.putItem("type", NS"QObjectChildList");
            d.putItemCount("value", objectChildren.size());
            d.putItem("numchild", objectChildren.size());
            d.endHash();
        }
        d.beginHash();
            d.putItem("name", "parent");
            qDumpInnerValueHelper(d, NS"QObject *", ob->parent());
        d.endHash();
#if 1
        d.beginHash();
            d.putItem("name", "className");
            d.putItem("value", ob->metaObject()->className());
            d.putItem("type", "");
            d.putItem("numchild", "0");
        d.endHash();
#endif
        d.endChildren();
    }
    d.disarm();
}

#if USE_QT_GUI
static const char *sizePolicyEnumValue(QSizePolicy::Policy p)
{
    switch (p) {
    case QSizePolicy::Fixed:
        return "Fixed";
    case QSizePolicy::Minimum:
        return "Minimum";
    case QSizePolicy::Maximum:
        return "Maximum";
    case QSizePolicy::Preferred:
        return "Preferred";
    case QSizePolicy::Expanding:
        return "Expanding";
    case QSizePolicy::MinimumExpanding:
        return "MinimumExpanding";
    case QSizePolicy::Ignored:
        break;
    }
    return "Ignored";
}

static QString sizePolicyValue(const QSizePolicy &sp)
{
    QString rc;
    QTextStream str(&rc);
    // Display as in Designer
    str << '[' << sizePolicyEnumValue(sp.horizontalPolicy())
        << ", " << sizePolicyEnumValue(sp.verticalPolicy())
        << ", " << sp.horizontalStretch() << ", " << sp.verticalStretch() << ']';
    return rc;
}
#endif

static void qDumpQVariantHelper(const QVariant *v, QString *value,
    QString *exp, int *numchild)
{
    switch (v->type()) {
    case QVariant::Invalid:
        *value = QLatin1String("<invalid>");
        *numchild = 0;
        break;
    case QVariant::String:
        *value = QLatin1Char('"') + v->toString() + QLatin1Char('"');
        *numchild = 0;
        break;
    #if QT_VERSION >= 0x040500
    case QVariant::StringList:
        *exp = QString(QLatin1String("(*('"NS"QStringList'*)%1)"))
                    .arg((quintptr)v);
        *numchild = v->toStringList().size();
        break;
    #endif
    case QVariant::Int:
        *value = QString::number(v->toInt());
        *numchild= 0;
        break;
    case QVariant::Double:
        *value = QString::number(v->toDouble());
        *numchild = 0;
        break;
    case QVariant::Point: {
            const QPoint p = v->toPoint();
            *value = QString::fromLatin1("%1, %2").arg(p.x()).arg(p.y());
        }
        *numchild = 0;
        break;
    case QVariant::Size: {
            const QSize size = v->toSize();
            *value = QString::fromLatin1("%1x%2")
                .arg(size.width()).arg(size.height());
        }
        *numchild = 0;
        break;
    case QVariant::Rect: {
            const QRect rect = v->toRect();
            *value = QString::fromLatin1("%1x%2+%3+%4")
                .arg(rect.width()).arg(rect.height())
                .arg(rect.x()).arg(rect.y());
        }
        *numchild = 0;
        break;
    case QVariant::PointF: {
            const QPointF p = v->toPointF();
            *value = QString::fromLatin1("%1, %2").arg(p.x()).arg(p.y());
        }
        *numchild = 0;
        break;

    case QVariant::SizeF: {
            const QSizeF size = v->toSizeF();
            *value = QString::fromLatin1("%1x%2")
                .arg(size.width()).arg(size.height());
        }
        *numchild = 0;
        break;
    case QVariant::RectF: {
            const QRectF rect = v->toRectF();
            *value = QString::fromLatin1("%1x%2+%3+%4")
                .arg(rect.width()).arg(rect.height())
                .arg(rect.x()).arg(rect.y());
        }
        *numchild = 0;
        break;
#if USE_QT_GUI
    case QVariant::Font:
        *value = qvariant_cast<QFont>(*v).toString();
        break;
    case QVariant::Color:
        *value = qvariant_cast<QColor>(*v).name();
        break;
    case QVariant::KeySequence:
        *value = qvariant_cast<QKeySequence>(*v).toString();
        break;
    case QVariant::SizePolicy:
        *value = sizePolicyValue(qvariant_cast<QSizePolicy>(*v));
        break;
#endif
    default: {
        char buf[1000];
        const char *format = (v->typeName()[0] == 'Q')
            ?  "'"NS"%s "NS"qVariantValue<"NS"%s >'(*('"NS"QVariant'*)%p)"
            :  "'%s "NS"qVariantValue<%s >'(*('"NS"QVariant'*)%p)";
        qsnprintf(buf, sizeof(buf) - 1, format, v->typeName(), v->typeName(), v);
        *exp = QLatin1String(buf);
        *numchild = 1;
        break;
        }
    }
}

static void qDumpQVariant(QDumper &d, const QVariant *v)
{
    QString value;
    QString exp;
    int numchild = 0;
    qDumpQVariantHelper(v, &value, &exp, &numchild);
    bool isInvalid = (v->typeName() == 0);
    if (isInvalid) {
        d.putItem("value", "(invalid)");
    } else if (value.isEmpty()) {
        d.beginItem("value");
            d.put("(").put(v->typeName()).put(") ");
        d.endItem();
    } else {
        QByteArray ba;
        ba += '(';
        ba += v->typeName();
        ba += ") ";
        ba += qPrintable(value);
        d.putItem("value", ba);
        d.putItem("valueencoded", "5");
    }
    d.putItem("type", NS"QVariant");
    if (isInvalid || !numchild) {
        d.putItem("numchild", "0");
    } else {
        d.putItem("numchild", "1");
        if (d.dumpChildren) {
            d.beginChildren();
            d.beginHash();
            d.putItem("name", "value");
            if (!exp.isEmpty())
                d.putItem("exp", qPrintable(exp));
            if (!value.isEmpty()) {
                d.putItem("value", value);
                d.putItem("valueencoded", "4");
            }
            d.putItem("type", v->typeName());
            d.putItem("numchild", numchild);
            d.endHash();
            d.endChildren();
        }
    }
    d.disarm();
}

static inline void qDumpQVariant(QDumper &d)
{
    qDumpQVariant(d, reinterpret_cast<const QVariant *>(d.data));
}

// Meta enumeration helpers
static inline void dumpMetaEnumType(QDumper &d, const QMetaEnum &me)
{
    QByteArray type = me.scope();
    if (!type.isEmpty())
        type += "::";
    type += me.name();
    d.putItem("type", type.constData());
}

static inline void dumpMetaEnumValue(QDumper &d, const QMetaProperty &mop,
                                     int value)
{

    const QMetaEnum me = mop.enumerator();
    dumpMetaEnumType(d, me);
    if (const char *enumValue = me.valueToKey(value)) {
        d.putItem("value", enumValue);
    } else {
        d.putItem("value", value);
    }
    d.putItem("numchild", 0);
}

static inline void dumpMetaFlagValue(QDumper &d, const QMetaProperty &mop,
                                     int value)
{
    const QMetaEnum me = mop.enumerator();
    dumpMetaEnumType(d, me);
    const QByteArray flagsValue = me.valueToKeys(value);
    if (flagsValue.isEmpty()) {
        d.putItem("value", value);
    } else {
        d.putItem("value", flagsValue.constData());
    }
    d.putItem("numchild", 0);
}

static void qDumpQObjectProperty(QDumper &d)
{
    const QObject *ob = (const QObject *)d.data;
    const QMetaObject *mob = ob->metaObject();
    // extract "local.Object.property"
    QString iname = d.iname;
    const int dotPos = iname.lastIndexOf(QLatin1Char('.'));
    if (dotPos == -1)
        return;
    iname.remove(0, dotPos + 1);
    const int index = mob->indexOfProperty(iname.toAscii());
    if (index == -1)
        return;
    const QMetaProperty mop = mob->property(index);
    const QVariant value = mop.read(ob);
    const bool isInteger = value.type() == QVariant::Int;
    if (isInteger && mop.isEnumType()) {
        dumpMetaEnumValue(d, mop, value.toInt());
    } else if (isInteger && mop.isFlagType()) {
        dumpMetaFlagValue(d, mop, value.toInt());
    } else {
        qDumpQVariant(d, &value);
    }
    d.disarm();
}

static void qDumpQObjectPropertyList(QDumper &d)
{
    const QObject *ob = (const QObject *)d.data;
    const QMetaObject *mo = ob->metaObject();
    const int propertyCount = mo->propertyCount();
    d.putItem("addr", "<synthetic>");
    d.putItem("type", NS"QObjectPropertyList");
    d.putItem("numchild", propertyCount);
    d.putItemCount("value", propertyCount);
    if (d.dumpChildren) {
        d.beginChildren();
        for (int i = propertyCount; --i >= 0; ) {
            const QMetaProperty & prop = mo->property(i);
            d.beginHash();
            d.putItem("name", prop.name());
            switch (prop.type()) {
            case QVariant::String:
                d.putItem("type", prop.typeName());
                d.putItem("value", prop.read(ob).toString());
                d.putItem("valueencoded", "2");
                d.putItem("numchild", "0");
                break;
            case QVariant::Bool:
                d.putItem("type", prop.typeName());
                d.putItem("value", (prop.read(ob).toBool() ? "true" : "false"));
                d.putItem("numchild", "0");
                break;
            case QVariant::Int:
                if (prop.isEnumType()) {
                    dumpMetaEnumValue(d, prop, prop.read(ob).toInt());
                } else if (prop.isFlagType()) {
                    dumpMetaFlagValue(d, prop, prop.read(ob).toInt());
                } else {
                    d.putItem("value", prop.read(ob).toInt());
                    d.putItem("numchild", "0");
                }
                break;
            default:
                d.putItem("addr", d.data);
                d.putItem("type", NS"QObjectProperty");
                d.putItem("numchild", "1");
                break;
            }
            d.endHash();
        }
        d.endChildren();
    }
    d.disarm();
}

static void qDumpQObjectMethodList(QDumper &d)
{
    const QObject *ob = (const QObject *)d.data;
    const QMetaObject *mo = ob->metaObject();
    d.putItem("addr", "<synthetic>");
    d.putItem("type", NS"QObjectMethodList");
    d.putItem("numchild", mo->methodCount());
    if (d.dumpChildren) {
        d.putItem("childtype", "QMetaMethod::Method");
        d.putItem("childnumchild", "0");
        d.beginChildren();
        for (int i = 0; i != mo->methodCount(); ++i) {
            const QMetaMethod & method = mo->method(i);
            int mt = method.methodType();
            d.beginHash();
            d.beginItem("name");
                d.put(i).put(" ").put(mo->indexOfMethod(method.signature()));
                d.put(" ").put(method.signature());
            d.endItem();
            d.beginItem("value");
                d.put((mt == QMetaMethod::Signal ? "<Signal>" : "<Slot>"));
                d.put(" (").put(mt).put(")");
            d.endItem();
            d.endHash();
        }
        d.endChildren();
    }
    d.disarm();
}

const char * qConnectionTypes[] ={
    "auto",
    "direct",
    "queued",
    "autocompat",
    "blockingqueued"
};

#if QT_VERSION >= 0x040400
static const ConnectionList &qConnectionList(const QObject *ob, int signalNumber)
{
    static const ConnectionList emptyList;
    const ObjectPrivate *p = reinterpret_cast<const ObjectPrivate *>(dfunc(ob));
    if (!p->connectionLists)
        return emptyList;
    typedef QVector<ConnectionList> ConnLists;
    const ConnLists *lists = reinterpret_cast<const ConnLists *>(p->connectionLists);
    // there's an optimization making the lists only large enough to hold the
    // last non-empty item
    if (signalNumber >= lists->size())
        return emptyList;
    return lists->at(signalNumber);
}
#endif

// Write party involved in a slot/signal element,
// avoid to recursion to self.
static inline void qDumpQObjectConnectionPart(QDumper &d,
                                              const QObject *owner,
                                              const QObject *partner,
                                              int number, const char *namePostfix)
{
    d.beginHash();
    d.beginItem("name");
    d.put(number).put(namePostfix);
    d.endItem();
    if (partner == owner) {
        d.putItem("value", QLatin1String("<this>"));
        d.putItem("valueencoded", "2");
        d.putItem("type", owner->metaObject()->className());
        d.putItem("numchild", 0);
        d.putItem("addr", owner);
    } else {
        qDumpInnerValueHelper(d, NS"QObject *", partner);
    }
    d.endHash();
}

static void qDumpQObjectSignal(QDumper &d)
{
    unsigned signalNumber = d.extraInt[0];

    d.putItem("addr", "<synthetic>");
    d.putItem("numchild", "1");
    d.putItem("type", NS"QObjectSignal");

#if QT_VERSION >= 0x040400
    if (d.dumpChildren) {
        const QObject *ob = reinterpret_cast<const QObject *>(d.data);
        d.beginChildren();
        const ConnectionList &connList = qConnectionList(ob, signalNumber);
        for (int i = 0; i != connList.size(); ++i) {
            const Connection &conn = connectionAt(connList, i);
            qDumpQObjectConnectionPart(d, ob, conn.receiver, i, " receiver");
            d.beginHash();
                d.beginItem("name");
                    d.put(i).put(" slot");
                d.endItem();
                d.putItem("type", "");
                if (conn.receiver)
                    d.putItem("value", conn.receiver->metaObject()->method(conn.method).signature());
                else
                    d.putItem("value", "<invalid receiver>");
                d.putItem("numchild", "0");
            d.endHash();
            d.beginHash();
                d.beginItem("name");
                    d.put(i).put(" type");
                d.endItem();
                d.putItem("type", "");
                d.beginItem("value");
                    d.put("<").put(qConnectionTypes[conn.method]).put(" connection>");
                d.endItem();
                d.putItem("numchild", "0");
            d.endHash();
        }
        d.endChildren();
        d.putItem("numchild", connList.size());
    }
#endif
    d.disarm();
}

static void qDumpQObjectSignalList(QDumper &d)
{
    const QObject *ob = reinterpret_cast<const QObject *>(d.data);
    const QMetaObject *mo = ob->metaObject();
    int count = 0;
    const int methodCount = mo->methodCount();
    for (int i = methodCount; --i >= 0; )
        count += (mo->method(i).methodType() == QMetaMethod::Signal);
    d.putItem("type", "QObjectSignalList");
    d.putItemCount("value", count);
    d.putItem("addr", d.data);
    d.putItem("numchild", count);
#if QT_VERSION >= 0x040400
    if (d.dumpChildren) {
        d.beginChildren();
        for (int i = 0; i != methodCount; ++i) {
            const QMetaMethod & method = mo->method(i);
            if (method.methodType() == QMetaMethod::Signal) {
                int k = mo->indexOfSignal(method.signature());
                const ConnectionList &connList = qConnectionList(ob, k);
                d.beginHash();
                d.putItem("name", k);
                d.putItem("value", method.signature());
                d.putItem("numchild", connList.size());
                d.putItem("addr", d.data);
                d.putItem("type", NS"QObjectSignal");
                d.endHash();
            }
        }
        d.endChildren();
    }
#endif
    d.disarm();
}

static void qDumpQObjectSlot(QDumper &d)
{
    int slotNumber = d.extraInt[0];

    d.putItem("addr", d.data);
    d.putItem("numchild", "1");
    d.putItem("type", NS"QObjectSlot");

#if QT_VERSION >= 0x040400
    if (d.dumpChildren) {
        d.beginChildren();
        int numchild = 0;
        const QObject *ob = reinterpret_cast<const QObject *>(d.data);
        const ObjectPrivate *p = reinterpret_cast<const ObjectPrivate *>(dfunc(ob));
        for (int s = 0; s != p->senders.size(); ++s) {
            const QObject *sender = senderAt(p->senders, s);
            int signal = signalAt(p->senders, s);
            const ConnectionList &connList = qConnectionList(sender, signal);
            for (int i = 0; i != connList.size(); ++i) {
                const Connection &conn = connectionAt(connList, i);
                if (conn.receiver == ob && conn.method == slotNumber) {
                    ++numchild;
                    const QMetaMethod &method = sender->metaObject()->method(signal);
                    qDumpQObjectConnectionPart(d, ob, sender, s, " sender");
                    d.beginHash();
                        d.beginItem("name");
                            d.put(s).put(" signal");
                        d.endItem();
                        d.putItem("type", "");
                        d.putItem("value", method.signature());
                        d.putItem("numchild", "0");
                    d.endHash();
                    d.beginHash();
                        d.beginItem("name");
                            d.put(s).put(" type");
                        d.endItem();
                        d.putItem("type", "");
                        d.beginItem("value");
                            d.put("<").put(qConnectionTypes[conn.method]);
                            d.put(" connection>");
                        d.endItem();
                        d.putItem("numchild", "0");
                    d.endHash();
                }
            }
        }
        d.endChildren();
        d.putItem("numchild", numchild);
    }
#endif
    d.disarm();
}

static void qDumpQObjectSlotList(QDumper &d)
{
    const QObject *ob = reinterpret_cast<const QObject *>(d.data);
#if QT_VERSION >= 0x040400
    const ObjectPrivate *p = reinterpret_cast<const ObjectPrivate *>(dfunc(ob));
#endif
    const QMetaObject *mo = ob->metaObject();

    int count = 0;
    const int methodCount = mo->methodCount();
    for (int i = methodCount; --i >= 0; )
        count += (mo->method(i).methodType() == QMetaMethod::Slot);

    d.putItem("numchild", count);
    d.putItemCount("value", count);
    d.putItem("type", NS"QObjectSlotList");
    if (d.dumpChildren) {
        d.beginChildren();
#if QT_VERSION >= 0x040400
        for (int i = 0; i != methodCount; ++i) {
            const QMetaMethod & method = mo->method(i);
            if (method.methodType() == QMetaMethod::Slot) {
                d.beginHash();
                int k = mo->indexOfSlot(method.signature());
                d.putItem("name", k);
                d.putItem("value", method.signature());

                // count senders. expensive...
                int numchild = 0;
                for (int s = 0; s != p->senders.size(); ++s) {
                    const QObject *sender = senderAt(p->senders, s);
                    int signal = signalAt(p->senders, s);
                    const ConnectionList &connList = qConnectionList(sender, signal);
                    for (int c = 0; c != connList.size(); ++c) {
                        const Connection &conn = connectionAt(connList, c);
                        if (conn.receiver == ob && conn.method == k)
                            ++numchild;
                    }
                }
                d.putItem("numchild", numchild);
                d.putItem("addr", d.data);
                d.putItem("type", NS"QObjectSlot");
                d.endHash();
            }
        }
#endif
        d.endChildren();
    }
    d.disarm();
}

static void qDumpQObjectChildList(QDumper &d)
{
    const QObject *ob = reinterpret_cast<const QObject *>(d.data);
    const QObjectList children = ob->children();
    const int size = children.size();

    d.putItem("numchild", size);
    d.putItemCount("value", size);
    d.putItem("type", NS"QObjectChildList");
    if (d.dumpChildren) {
        d.beginChildren();
        for (int i = 0; i != size; ++i) {
            d.beginHash();
            d.putItem("name", i);
            qDumpInnerValueHelper(d, NS"QObject *", children.at(i));
            d.endHash();
        }
        d.endChildren();
    }
    d.disarm();
}

#if USE_QT_GUI
static void qDumpQPixmap(QDumper &d)
{
    const QPixmap &im = *reinterpret_cast<const QPixmap *>(d.data);
    d.beginItem("value");
        d.put("(").put(im.width()).put("x").put(im.height()).put(")");
    d.endItem();
    d.putItem("type", NS"QPixmap");
    d.putItem("numchild", "0");
    d.disarm();
}
#endif

static void qDumpQSet(QDumper &d)
{
    // This uses the knowledge that QHash<T> has only a single member
    // of  union { QHashData *d; QHashNode<Key, T> *e; };
    QHashData *hd = *(QHashData**)d.data;
    QHashData::Node *node = hd->firstNode();

    int n = hd->size;
    if (n < 0)
        return;
    if (n > 0) {
        qCheckAccess(node);
        qCheckPointer(node->next);
    }

    d.putItemCount("value", n);
    d.putItem("valuedisabled", "true");
    d.putItem("numchild", 2 * n);
    if (d.dumpChildren) {
        if (n > 100)
            n = 100;
        d.beginChildren();
        int i = 0;
        for (int bucket = 0; bucket != hd->numBuckets && i <= 10000; ++bucket) {
            for (node = hd->buckets[bucket]; node->next; node = node->next) {
                d.beginHash();
                d.putItem("name", i);
                d.putItem("type", d.innertype);
                d.beginItem("exp");
                    d.put("(('"NS"QHashNode<").put(d.innertype
                   ).put(","NS"QHashDummyValue>'*)"
                   ).put(static_cast<const void*>(node)).put(")->key");
                d.endItem();
                d.endHash();
                ++i;
                if (i > 10000) {
                    d.putEllipsis();
                    break;
                }
            }
        }
        d.endChildren();
    }
    d.disarm();
}

#if QT_VERSION >= 0x040500
static void qDumpQSharedPointer(QDumper &d)
{
    const QSharedPointer<int> &ptr =
        *reinterpret_cast<const QSharedPointer<int> *>(d.data);

    if (isSimpleType(d.innertype))
        qDumpInnerValueHelper(d, d.innertype, ptr.data());
    else
        d.putItem("value", "");
    d.putItem("valuedisabled", "true");
    d.putItem("numchild", 1);
    if (d.dumpChildren) {
        d.beginChildren();
        d.beginHash();
            d.putItem("name", "data");
            qDumpInnerValue(d, d.innertype, ptr.data());
        d.endHash();
        const int v = sizeof(void *);
        d.beginHash();
            const void *weak = addOffset(deref(addOffset(d.data, v)), v);
            d.putItem("name", "weakref");
            d.putItem("value", *static_cast<const int *>(weak));
            d.putItem("type", "int");
            d.putItem("addr",  weak);
            d.putItem("numchild", "0");
        d.endHash();
        d.beginHash();
            const void *strong = addOffset(weak, sizeof(int));
            d.putItem("name", "strongref");
            d.putItem("value", *static_cast<const int *>(strong));
            d.putItem("type", "int");
            d.putItem("addr",  strong);
            d.putItem("numchild", "0");
        d.endHash();
        d.endChildren();
    }
    d.disarm();
}
#endif // QT_VERSION >= 0x040500

static void qDumpQString(QDumper &d)
{
    const QString &str = *reinterpret_cast<const QString *>(d.data);

    const int size = str.size();
    if (size < 0)
        return;
    if (size) {
        const QChar *unicode = str.unicode();
        qCheckAccess(unicode);
        qCheckAccess(unicode + size);
        if (!unicode[size].isNull()) // must be '\0' terminated
            return;
    }

    d.putItem("value", str);
    d.putItem("valueencoded", "2");
    d.putItem("type", NS"QString");
    //d.putItem("editvalue", str);  // handled generically below
    d.putItem("numchild", "0");

    d.disarm();
}

static void qDumpQStringList(QDumper &d)
{
    const QStringList &list = *reinterpret_cast<const QStringList *>(d.data);
    int n = list.size();
    if (n < 0)
        return;
    if (n > 0) {
        qCheckAccess(&list.front());
        qCheckAccess(&list.back());
    }

    d.putItemCount("value", n);
    d.putItem("valuedisabled", "true");
    d.putItem("numchild", n);
    if (d.dumpChildren) {
        d.putItem("childtype", NS"QString");
        d.putItem("childnumchild", "0");
        if (n > 1000)
            n = 1000;
        d.beginChildren();
        for (int i = 0; i != n; ++i) {
            d.beginHash();
            d.putItem("name", i);
            d.putItem("value", list[i]);
            d.putItem("valueencoded", "2");
            d.endHash();
        }
        if (n < list.size())
            d.putEllipsis();
        d.endChildren();
    }
    d.disarm();
}

static void qDumpQTextCodec(QDumper &d)
{
    const QTextCodec &codec = *reinterpret_cast<const QTextCodec *>(d.data);
    d.putItem("value", codec.name());
    d.putItem("valueencoded", "1");
    d.putItem("type", NS"QTextCodec");
    d.putItem("numchild", "2");
    if (d.dumpChildren) {
        d.beginChildren();
        d.putHash("name", codec.name());
        d.putHash("mibEnum", codec.mibEnum());
        d.endChildren();
    }
    d.disarm();
}


static void qDumpQVector(QDumper &d)
{
    QVectorData *v = *reinterpret_cast<QVectorData *const*>(d.data);

    // Try to provoke segfaults early to prevent the frontend
    // from asking for unavailable child details
    int nn = v->size;
    if (nn < 0)
        return;
    if (nn > 0) {
        //qCheckAccess(&vec.front());
        //qCheckAccess(&vec.back());
    }

    unsigned innersize = d.extraInt[0];
    unsigned typeddatasize = d.extraInt[1];

    int n = nn;
    d.putItemCount("value", n);
    d.putItem("valuedisabled", "true");
    d.putItem("numchild", n);
    InnerValueResult innerValueResult = InnerValueChildrenSpecified;
    if (d.dumpChildren) {
        QByteArray strippedInnerType = stripPointerType(d.innertype);
        const char *stripped =
            isPointerType(d.innertype) ? strippedInnerType.data() : 0;
        if (n > 1000)
            n = 1000;
        d.beginChildren();
        for (int i = 0; i != n; ++i) {
            d.beginHash();
            d.putItem("name", i);
            innerValueResult = qDumpInnerValueOrPointer(d, d.innertype, stripped,
                addOffset(v, i * innersize + typeddatasize));
            d.endHash();
        }
        if (n < nn)
            d.putEllipsis();
        d.endChildren();
    }
    dumpChildNumChildren(d, innerValueResult);
    d.disarm();
}

#if QT_VERSION >= 0x040500
static void qDumpQWeakPointer(QDumper &d)
{
    const int v = sizeof(void *);
    const void *value = deref(addOffset(d.data, v));

    if (isSimpleType(d.innertype))
        qDumpInnerValueHelper(d, d.innertype, value);
    else
        d.putItem("value", "");
    d.putItem("valuedisabled", "true");
    d.putItem("numchild", 1);
    if (d.dumpChildren) {
        d.beginChildren();
        d.beginHash();
            d.putItem("name", "data");
            qDumpInnerValue(d, d.innertype, value);
        d.endHash();
        d.beginHash();
            const void *weak = addOffset(deref(d.data), v);
            d.putItem("name", "weakref");
            d.putItem("value", *static_cast<const int *>(weak));
            d.putItem("type", "int");
            d.putItem("addr",  weak);
            d.putItem("numchild", "0");
        d.endHash();
        d.beginHash();
            const void *strong = addOffset(weak, sizeof(int));
            d.putItem("name", "strongref");
            d.putItem("value", *static_cast<const int *>(strong));
            d.putItem("type", "int");
            d.putItem("addr",  strong);
            d.putItem("numchild", "0");
        d.endHash();
        d.endChildren();
    }
    d.disarm();
}
#endif // QT_VERSION >= 0x040500

static void qDumpStdList(QDumper &d)
{
    const std::list<int> &list = *reinterpret_cast<const std::list<int> *>(d.data);
#ifdef Q_CC_MSVC
    const int size = static_cast<int>(list.size());
    if (size < 0)
        return;
    if (size)
        qCheckAccess(list.begin().operator ->());
#else
    const void *p = d.data;
    qCheckAccess(p);
    p = deref(p);
    qCheckAccess(p);
    p = deref(p);
    qCheckAccess(p);
    p = deref(addOffset(d.data, sizeof(void*)));
    qCheckAccess(p);
    p = deref(addOffset(p, sizeof(void*)));
    qCheckAccess(p);
    p = deref(addOffset(p, sizeof(void*)));
    qCheckAccess(p);
#endif
    int nn = 0;
    std::list<int>::const_iterator it = list.begin();
    const std::list<int>::const_iterator cend = list.end();
    for (; nn < 101 && it != cend; ++nn, ++it)
        qCheckAccess(it.operator->());

    if (nn > 100)
        d.putItem("value", "<more than 100 items>");
    else
        d.putItemCount("value", nn);
    d.putItem("numchild", nn);

    d.putItem("valuedisabled", "true");
    InnerValueResult innerValueResult = InnerValueChildrenSpecified;
    if (d.dumpChildren) {
        QByteArray strippedInnerType = stripPointerType(d.innertype);
        const char *stripped =
            isPointerType(d.innertype) ? strippedInnerType.data() : 0;
        d.beginChildren();
        it = list.begin();
        for (int i = 0; i < 1000 && it != cend; ++i, ++it) {
            d.beginHash();
            d.putItem("name", i);
            innerValueResult = qDumpInnerValueOrPointer(d, d.innertype, stripped, it.operator->());
            d.endHash();
        }
        if (it != list.end())
            d.putEllipsis();
        d.endChildren();
    }
    dumpChildNumChildren(d, innerValueResult);
    d.disarm();
}

/* Dump out an arbitrary map. To iterate the map,
 * it is cast to a map of <KeyType,Value>. 'int' can be used for both
 * for all types if the implementation does not depend on the types
 * which is the case for GNU STL. The implementation used by MS VC, however,
 * does depend on the key/value type, so, special cases need to be hardcoded. */

template <class KeyType, class ValueType>
static void qDumpStdMapHelper(QDumper &d)
{
    typedef std::map<KeyType, ValueType> DummyType;
    const DummyType &map = *reinterpret_cast<const DummyType*>(d.data);
    const char *keyType   = d.templateParameters[0];
    const char *valueType = d.templateParameters[1];
    const void *p = d.data;
    qCheckAccess(p);
    p = deref(p);

    const int nn = map.size();
    if (nn < 0)
        return;
    Q_TYPENAME DummyType::const_iterator it = map.begin();
    const Q_TYPENAME DummyType::const_iterator cend = map.end();
    for (int i = 0; i < nn && i < 10 && it != cend; ++i, ++it)
        qCheckAccess(it.operator->());

    const QByteArray strippedInnerType = stripPointerType(d.innertype);
    d.putItem("numchild", nn);
    d.putItemCount("value", nn);
    d.putItem("valuedisabled", "true");
    d.putItem("valueoffset", d.extraInt[2]);

    // HACK: we need a properly const qualified version of the
    // std::pair used. We extract it from the allocator parameter
    // (#4, "std::allocator<std::pair<key, value> >")
    // as it is there, and, equally importantly, in an order that
    // gdb accepts when fed with it.
    char *pairType = (char *)(d.templateParameters[3]) + 15;
    pairType[strlen(pairType) - 2] = 0;
    d.putItem("pairtype", pairType);

    InnerValueResult innerValueResult = InnerValueChildrenSpecified;
    if (d.dumpChildren) {
        bool isSimpleKey = isSimpleType(keyType);
        bool isSimpleValue = isSimpleType(valueType);
        int valueOffset = d.extraInt[2];

        d.beginItem("extra");
            d.put("isSimpleKey: ").put(isSimpleKey);
            d.put(" isSimpleValue: ").put(isSimpleValue);
            d.put(" valueType: '").put(valueType);
            d.put(" valueOffset: ").put(valueOffset);
        d.endItem();

        d.beginChildren();
        it = map.begin();
        for (int i = 0; i < 1000 && it != cend; ++i, ++it) {
            d.beginHash();
                const void *node = it.operator->();
                d.putItem("name", i);
                qDumpInnerValueHelper(d, keyType, node, "key");
                innerValueResult = qDumpInnerValueHelper(d, valueType, addOffset(node, valueOffset));
                if (isSimpleKey && isSimpleValue) {
                    d.putItem("type", valueType);
                    d.putItem("addr", addOffset(node, valueOffset));
                    d.putItem("numchild", 0);
                } else {
                    d.putItem("addr", node);
                    d.putItem("type", pairType);
                    d.putItem("numchild", 2);
                }
            d.endHash();
        }
        if (it != map.end())
            d.putEllipsis();
        d.endChildren();
    }
    dumpChildNumChildren(d, innerValueResult);
    d.disarm();
}

static void qDumpStdMap(QDumper &d)
{
#ifdef Q_CC_MSVC
    // As the map implementation inherits from a base class
    // depending on the key, use something equivalent to iterate it.
    const int keySize = d.extraInt[0];
    const int valueSize = d.extraInt[1];
    if (keySize == valueSize) {
        if (keySize == sizeof(int)) {
            qDumpStdMapHelper<int,int>(d);
            return;
        }
        if (keySize == sizeof(std::string)) {
            qDumpStdMapHelper<std::string,std::string>(d);
            return;
        }
        return;
    }
    if (keySize == sizeof(int) && valueSize == sizeof(std::string)) {
        qDumpStdMapHelper<int,std::string>(d);
        return;
    }
    if (keySize == sizeof(std::string) && valueSize == sizeof(int)) {
        qDumpStdMapHelper<std::string,int>(d);
        return;
    }
#else
    qDumpStdMapHelper<int,int>(d);
#endif
}

/* Dump out an arbitrary set. To iterate the set,
 * it is cast to a set of <KeyType>. 'int' can be used
 * for all types if the implementation does not depend on the key type
 * which is the case for GNU STL. The implementation used by MS VC, however,
 * does depend on the key type, so, special cases need to be hardcoded. */

template <class KeyType>
static void qDumpStdSetHelper(QDumper &d)
{
    typedef std::set<KeyType> DummyType;
    const DummyType &set = *reinterpret_cast<const DummyType*>(d.data);
    const void *p = d.data;
    qCheckAccess(p);
    p = deref(p);

    const int nn = set.size();
    if (nn < 0)
        return;
    Q_TYPENAME DummyType::const_iterator it = set.begin();
    const Q_TYPENAME DummyType::const_iterator cend = set.end();
    for (int i = 0; i < nn && i < 10 && it != cend; ++i, ++it)
        qCheckAccess(it.operator->());

    d.putItemCount("value", nn);
    d.putItem("valuedisabled", "true");
    d.putItem("numchild", nn);
    d.putItem("valueoffset", d.extraInt[0]);

    if (d.dumpChildren) {
        InnerValueResult innerValueResult = InnerValueChildrenSpecified;
        int valueOffset = 0; // d.extraInt[0];
        QByteArray strippedInnerType = stripPointerType(d.innertype);
        const char *stripped =
            isPointerType(d.innertype) ? strippedInnerType.data() : 0;

        d.beginItem("extra");
            d.put("valueOffset: ").put(valueOffset);
        d.endItem();

        d.beginChildren();
        it = set.begin();
        for (int i = 0; i < 1000 && it != cend; ++i, ++it) {
            const void *node = it.operator->();
            d.beginHash();
            d.putItem("name", i);
            innerValueResult = qDumpInnerValueOrPointer(d, d.innertype, stripped, node);
            d.endHash();
        }
        if (it != set.end())
            d.putEllipsis();
        d.endChildren();
        dumpChildNumChildren(d, innerValueResult);
    }
    d.disarm();
}

static void qDumpStdSet(QDumper &d)
{
#ifdef Q_CC_MSVC
    // As the set implementation inherits from a base class
    // depending on the key, use something equivalent to iterate it.
    const int innerSize = d.extraInt[0];
    if (innerSize == sizeof(int)) {
        qDumpStdSetHelper<int>(d);
        return;
    }
    if (innerSize == sizeof(std::string)) {
        qDumpStdSetHelper<std::string>(d);
        return;
    }
    if (innerSize == sizeof(std::wstring)) {
        qDumpStdSetHelper<std::wstring>(d);
        return;
    }
#else
    qDumpStdSetHelper<int>(d);
#endif
}

static void qDumpStdString(QDumper &d)
{
    const std::string &str = *reinterpret_cast<const std::string *>(d.data);

    const std::string::size_type size = str.size();
    if (int(size) < 0)
        return;
    if (size) {
        qCheckAccess(str.c_str());
        qCheckAccess(str.c_str() + size - 1);
    }
    dumpStdStringValue(d, str);
    d.disarm();
}

static void qDumpStdWString(QDumper &d)
{
    const std::wstring &str = *reinterpret_cast<const std::wstring *>(d.data);
    const std::wstring::size_type size = str.size();
    if (int(size) < 0)
        return;
    if (size) {
        qCheckAccess(str.c_str());
        qCheckAccess(str.c_str() + size - 1);
    }
    dumpStdWStringValue(d, str);
    d.disarm();
}

static void qDumpStdVector(QDumper &d)
{
    // Correct type would be something like:
    // std::_Vector_base<int,std::allocator<int, std::allocator<int> >>::_Vector_impl
    struct VectorImpl {
        char *start;
        char *finish;
        char *end_of_storage;
    };
#ifdef Q_CC_MSVC
    // Pointers are at end of the structure
    const char * vcp = static_cast<const char *>(d.data);
    const VectorImpl *v = reinterpret_cast<const VectorImpl *>(vcp + sizeof(std::vector<int>) - sizeof(VectorImpl));
#else
    const VectorImpl *v = static_cast<const VectorImpl *>(d.data);
#endif
    // Try to provoke segfaults early to prevent the frontend
    // from asking for unavailable child details
    int nn = (v->finish - v->start) / d.extraInt[0];
    if (nn < 0)
        return;
    if (nn > 0) {
        qCheckAccess(v->start);
        qCheckAccess(v->finish);
        qCheckAccess(v->end_of_storage);
    }

    int n = nn;
    d.putItemCount("value", n);
    d.putItem("valuedisabled", "true");
    d.putItem("numchild", n);
    if (d.dumpChildren) {
        InnerValueResult innerValueResult = InnerValueChildrenSpecified;
        unsigned innersize = d.extraInt[0];
        QByteArray strippedInnerType = stripPointerType(d.innertype);
        const char *stripped =
            isPointerType(d.innertype) ? strippedInnerType.data() : 0;
        if (n > 1000)
            n = 1000;
        d.beginChildren();
        for (int i = 0; i != n; ++i) {
            d.beginHash();
            d.putItem("name", i);
            qDumpInnerValueOrPointer(d, d.innertype, stripped,
                addOffset(v->start, i * innersize));
            d.endHash();
        }
        if (n < nn)
            d.putEllipsis();
        d.endChildren();
        dumpChildNumChildren(d, innerValueResult);
    }
    d.disarm();
}

static void qDumpStdVectorBool(QDumper &d)
{
    // FIXME
    return qDumpStdVector(d);
}

static void handleProtocolVersion2and3(QDumper & d)
{
    if (!d.outertype[0]) {
        qDumpUnknown(d);
        return;
    }
#ifdef Q_CC_MSVC // Catch exceptions with MSVC/CDB
    __try {
#endif

    d.setupTemplateParameters();
    d.putItem("iname", d.iname);
    if (d.data)
        d.putItem("addr", d.data);

#ifdef QT_NO_QDATASTREAM
    if (d.protocolVersion == 3) {
        QVariant::Type type = QVariant::nameToType(d.outertype);
        if (type != QVariant::Invalid) {
            QVariant v(type, d.data);
            QByteArray ba;
            QDataStream ds(&ba, QIODevice::WriteOnly);
            ds << v;
            d.putItem("editvalue", ba);
        }
    }
#endif

    const char *type = stripNamespace(d.outertype);
    // type[0] is usally 'Q', so don't use it
    switch (type[1]) {
        case 'a':
            if (isEqual(type, "map"))
                qDumpStdMap(d);
            break;
        case 'A':
            if (isEqual(type, "QAbstractItemModel"))
                qDumpQAbstractItemModel(d);
            else if (isEqual(type, "QAbstractItem"))
                qDumpQAbstractItem(d);
            break;
        case 'B':
            if (isEqual(type, "QByteArray"))
                qDumpQByteArray(d);
            break;
        case 'C':
            if (isEqual(type, "QChar"))
                qDumpQChar(d);
            break;
        case 'D':
            if (isEqual(type, "QDateTime"))
                qDumpQDateTime(d);
            else if (isEqual(type, "QDir"))
                qDumpQDir(d);
            break;
        case 'e':
            if (isEqual(type, "vector"))
                qDumpStdVector(d);
            else if (isEqual(type, "set"))
                qDumpStdSet(d);
            break;
        case 'F':
            if (isEqual(type, "QFile"))
                qDumpQFile(d);
            else if (isEqual(type, "QFileInfo"))
                qDumpQFileInfo(d);
            break;
        case 'H':
            if (isEqual(type, "QHash"))
                qDumpQHash(d);
            else if (isEqual(type, "QHashNode"))
                qDumpQHashNode(d);
            break;
        case 'i':
            if (isEqual(type, "list"))
                qDumpStdList(d);
            break;
        case 'I':
            #if USE_QT_GUI
            if (isEqual(type, "QImage"))
                qDumpQImage(d);
            else if (isEqual(type, "QImageData"))
                qDumpQImageData(d);
            #endif
            break;
        case 'L':
            if (isEqual(type, "QList"))
                qDumpQList(d);
            else if (isEqual(type, "QLinkedList"))
                qDumpQLinkedList(d);
            else if (isEqual(type, "QLocale"))
                qDumpQLocale(d);
            break;
        case 'M':
            if (isEqual(type, "QMap"))
                qDumpQMap(d);
            else if (isEqual(type, "QMapNode"))
                qDumpQMapNode(d);
            else if (isEqual(type, "QModelIndex"))
                qDumpQModelIndex(d);
            else if (isEqual(type, "QMultiMap"))
                qDumpQMultiMap(d);
            break;
        case 'O':
            if (isEqual(type, "QObject"))
                qDumpQObject(d);
            else if (isEqual(type, "QObjectPropertyList"))
                qDumpQObjectPropertyList(d);
            else if (isEqual(type, "QObjectProperty"))
                qDumpQObjectProperty(d);
            else if (isEqual(type, "QObjectMethodList"))
                qDumpQObjectMethodList(d);
            else if (isEqual(type, "QObjectSignal"))
                qDumpQObjectSignal(d);
            else if (isEqual(type, "QObjectSignalList"))
                qDumpQObjectSignalList(d);
            else if (isEqual(type, "QObjectSlot"))
                qDumpQObjectSlot(d);
            else if (isEqual(type, "QObjectSlotList"))
                qDumpQObjectSlotList(d);
            else if (isEqual(type, "QObjectChildList"))
                qDumpQObjectChildList(d);
            break;
        case 'P':
            #if USE_QT_GUI
            if (isEqual(type, "QPixmap"))
                qDumpQPixmap(d);
            #endif
            break;
        case 'S':
            if (isEqual(type, "QSet"))
                qDumpQSet(d);
            #if QT_VERSION >= 0x040500
            else if (isEqual(type, "QSharedPointer"))
                qDumpQSharedPointer(d);
            #endif
            else if (isEqual(type, "QString"))
                qDumpQString(d);
            else if (isEqual(type, "QStringList"))
                qDumpQStringList(d);
            break;
        case 's':
            if (isEqual(type, "wstring"))
                qDumpStdWString(d);
            break;
        case 't':
            if (isEqual(type, "std::vector"))
                qDumpStdVector(d);
            else if (isEqual(type, "std::vector::bool"))
                qDumpStdVectorBool(d);
            else if (isEqual(type, "std::list"))
                qDumpStdList(d);
            else if (isEqual(type, "std::map"))
                qDumpStdMap(d);
            else if (isEqual(type, "std::set"))
                qDumpStdSet(d);
            else if (isEqual(type, "std::string") || isEqual(type, "string"))
                qDumpStdString(d);
            else if (isEqual(type, "std::wstring"))
                qDumpStdWString(d);
            break;
        case 'T':
            if (isEqual(type, "QTextCodec"))
                qDumpQTextCodec(d);
            break;
        case 'V':
            if (isEqual(type, "QVariant"))
                qDumpQVariant(d);
            else if (isEqual(type, "QVector"))
                qDumpQVector(d);
            break;
        case 'W':
            #if QT_VERSION >= 0x040500
            if (isEqual(type, "QWeakPointer"))
                qDumpQWeakPointer(d);
            #endif
            break;
    }

    if (!d.success)
        qDumpUnknown(d);
#ifdef Q_CC_MSVC // Catch exceptions with MSVC/CDB
    } __except(EXCEPTION_EXECUTE_HANDLER) {
        qDumpUnknown(d, DUMPUNKNOWN_MESSAGE" <exception>");
    }
#endif

}

} // anonymous namespace


#if USE_QT_GUI
extern "C" Q_DECL_EXPORT
void *watchPoint(int x, int y)
{
    return QApplication::widgetAt(x, y);
}
#endif

// Helper to write out common expression values for CDB:
// Offsets of a map node value which looks like
// "(size_t)&(('QMapNode<QString,QString >'*)0)->value")" in gdb syntax

template <class Key, class Value>
        inline QDumper & putQMapNodeOffsetExpression(const char *keyType,
                                                     const char *valueType,
                                                     QDumper &d)
{
    QMapNode<Key, Value> *mn = 0;
    const int valueOffset = (char *)&(mn->value) - (char*)mn;
    d.put("(size_t)&(('"NS"QMapNode<");
    d.put(keyType);
    d.put(',');
    d.put(valueType);
    if (valueType[qstrlen(valueType) - 1] == '>')
        d.put(' ');
    d.put(">'*)0)->value=\"");
    d.put(valueOffset);
    d.put('"');
    return d;
}

// Helper to write out common expression values for CDB:
// Offsets of a std::pair for dumping std::map node value which look like
// "(size_t)&(('std::pair<int const ,unsigned int>'*)0)->second"

template <class Key, class Value>
        inline QDumper & putStdPairValueOffsetExpression(const char *keyType,
                                                         const char *valueType,
                                                         QDumper &d)
{
    std::pair<Key, Value> *p = 0;
    const int valueOffset = (char *)&(p->second) - (char*)p;
    d.put("(size_t)&(('std::pair<");
    d.put(keyType);
    d.put(" const ,");
    d.put(valueType);
    if (valueType[qstrlen(valueType) - 1] == '>')
        d.put(' ');
    d.put(">'*)0)->second=\"");
    d.put(valueOffset);
    d.put('"');
    return  d;
}

extern "C" Q_DECL_EXPORT
void *qDumpObjectData440(
    int protocolVersion,
    int token,
    void *data,
    int dumpChildren,
    int extraInt0,
    int extraInt1,
    int extraInt2,
    int extraInt3)
{
    //sleep(20);
    if (protocolVersion == 1) {
        QDumper d;
        d.protocolVersion = protocolVersion;
        d.token           = token;

        // This is a list of all available dumpers. Note that some templates
        // currently require special hardcoded handling in the debugger plugin.
        // They are mentioned here nevertheless. For types that are not listed
        // here, dumpers won't be used.
        d.put("dumpers=["
            "\""NS"QAbstractItem\","
            "\""NS"QAbstractItemModel\","
            "\""NS"QByteArray\","
            "\""NS"QChar\","
            "\""NS"QDateTime\","
            "\""NS"QDir\","
            "\""NS"QFile\","
            "\""NS"QFileInfo\","
            "\""NS"QHash\","
            "\""NS"QHashNode\","
            "\""NS"QImage\","
            "\""NS"QImageData\","
            "\""NS"QLinkedList\","
            "\""NS"QList\","
            "\""NS"QLocale\","
            "\""NS"QMap\","
            "\""NS"QMapNode\","
            "\""NS"QModelIndex\","
            "\""NS"QObject\","
            "\""NS"QObjectMethodList\","   // hack to get nested properties display
            "\""NS"QObjectProperty\","
            "\""NS"QObjectPropertyList\","
            "\""NS"QObjectSignal\","
            "\""NS"QObjectSignalList\","
            "\""NS"QObjectSlot\","
            "\""NS"QObjectSlotList\","
            "\""NS"QObjectChildList\","
            //"\""NS"QRegion\","
            "\""NS"QSet\","
            "\""NS"QString\","
            "\""NS"QStringList\","
            "\""NS"QTextCodec\","
            "\""NS"QVariant\","
            "\""NS"QVector\","
#if QT_VERSION >= 0x040500
            "\""NS"QMultiMap\","
            "\""NS"QSharedPointer\","
            "\""NS"QWeakPointer\","
#endif
#if USE_QT_GUI
            "\""NS"QWidget\","
#endif
#ifdef Q_OS_WIN
            "\"basic_string\","
            "\"list\","
            "\"map\","
            "\"set\","
            "\"string\","
            "\"vector\","
            "\"wstring\","
#endif
            "\"std::basic_string\","
            "\"std::list\","
            "\"std::map\","
            "\"std::set\","
            "\"std::string\","
            "\"std::vector\","
            "\"std::wstring\","
            "]");
        d.put(",qtversion=["
            "\"").put(((QT_VERSION >> 16) & 255)).put("\","
            "\"").put(((QT_VERSION >> 8)  & 255)).put("\","
            "\"").put(((QT_VERSION)       & 255)).put("\"]");
        d.put(",dumperversion=\"1.3\",");
//      Dump out size information
        d.put("sizes={");
        d.put("int=\"").put(sizeof(int)).put("\",")
         .put("char*=\"").put(sizeof(char*)).put("\",")
         .put(""NS"QString=\"").put(sizeof(QString)).put("\",")
         .put(""NS"QStringList=\"").put(sizeof(QStringList)).put("\",")
         .put(""NS"QObject=\"").put(sizeof(QObject)).put("\",")
#if USE_QT_GUI
         .put(""NS"QWidget=\"").put(sizeof(QWidget)).put("\",")
#endif
#ifdef Q_OS_WIN
         .put("string=\"").put(sizeof(std::string)).put("\",")
         .put("wstring=\"").put(sizeof(std::wstring)).put("\",")
#endif
         .put("std::string=\"").put(sizeof(std::string)).put("\",")
         .put("std::wstring=\"").put(sizeof(std::wstring)).put("\",")
         .put("std::allocator=\"").put(sizeof(std::allocator<int>)).put("\",")
         .put("std::char_traits<char>=\"").put(sizeof(std::char_traits<char>)).put("\",")
         .put("std::char_traits<unsigned short>=\"").put(sizeof(std::char_traits<unsigned short>)).put("\",")
#if QT_VERSION >= 0x040500
         .put(NS"QSharedPointer=\"").put(sizeof(QSharedPointer<int>)).put("\",")
         .put(NS"QSharedDataPointer=\"").put(sizeof(QSharedDataPointer<QSharedData>)).put("\",")
         .put(NS"QWeakPointer=\"").put(sizeof(QWeakPointer<int>)).put("\",")
#endif
         .put("QPointer=\"").put(sizeof(QPointer<QObject>)).put("\",")
         // Common map node types
         .put(NS"QMapNode<int,int>=\"").put(sizeof(QMapNode<int,int >)).put("\",")
         .put(NS"QMapNode<int,"NS"QString>=\"").put(sizeof(QMapNode<int, QString>)).put("\",")
         .put(NS"QMapNode<int,"NS"QVariant>=\"").put(sizeof(QMapNode<int, QVariant>)).put("\",")
         .put(NS"QMapNode<"NS"QString,int>=\"").put(sizeof(QMapNode<QString, int>)).put("\",")
         .put(NS"QMapNode<"NS"QString,"NS"QString>=\"").put(sizeof(QMapNode<QString, QString>)).put("\",")
         .put(NS"QMapNode<"NS"QString,"NS"QVariant>=\"").put(sizeof(QMapNode<QString, QVariant>))
         .put("\"}");
        // Write out common expression values for CDB
        d.put(",expressions={");
        putQMapNodeOffsetExpression<int,int>("int", "int", d).put(',');
        putQMapNodeOffsetExpression<int,QString>("int", NS"QString", d).put(',');
        putQMapNodeOffsetExpression<int,QVariant>("int", NS"QVariant", d).put(',');
        putQMapNodeOffsetExpression<QString,int>(NS"QString", "int", d).put(',');
        putQMapNodeOffsetExpression<QString,QString>(NS"QString", NS"QString", d).put(',');
        putQMapNodeOffsetExpression<QString,QVariant>(NS"QString", NS"QVariant", d).put(',');
        // Std Pairs
        putStdPairValueOffsetExpression<int,int>("int","int", d).put(',');
        putStdPairValueOffsetExpression<QString,QString>(NS"QString",NS"QString", d).put(',');
        putStdPairValueOffsetExpression<int,QString>("int",NS"QString", d).put(',');
        putStdPairValueOffsetExpression<QString,int>(NS"QString", "int", d).put(',');
        putStdPairValueOffsetExpression<std::string,std::string>(stdStringTypeC, stdStringTypeC, d).put(',');
        putStdPairValueOffsetExpression<int,std::string>("int", stdStringTypeC, d).put(',');
        putStdPairValueOffsetExpression<std::string,int>(stdStringTypeC, "int", d.put(','));
        putStdPairValueOffsetExpression<std::wstring,std::wstring>(stdWideStringTypeUShortC, stdWideStringTypeUShortC, d).put(',');
        putStdPairValueOffsetExpression<int,std::wstring>("int", stdWideStringTypeUShortC, d).put(',');
        putStdPairValueOffsetExpression<std::wstring,int>(stdWideStringTypeUShortC, "int", d);
        d.put('}');
        d.disarm();
    }

    else if (protocolVersion == 2 || protocolVersion == 3) {
        QDumper d;

        d.protocolVersion = protocolVersion;
        d.token           = token;
        d.data            = data;
        d.dumpChildren    = dumpChildren;
        d.extraInt[0]     = extraInt0;
        d.extraInt[1]     = extraInt1;
        d.extraInt[2]     = extraInt2;
        d.extraInt[3]     = extraInt3;

        const char *inbuffer = inBuffer;
        d.outertype = inbuffer; while (*inbuffer) ++inbuffer; ++inbuffer;
        d.iname     = inbuffer; while (*inbuffer) ++inbuffer; ++inbuffer;
        d.exp       = inbuffer; while (*inbuffer) ++inbuffer; ++inbuffer;
        d.innertype = inbuffer; while (*inbuffer) ++inbuffer; ++inbuffer;
        d.iname     = inbuffer; while (*inbuffer) ++inbuffer; ++inbuffer;

        handleProtocolVersion2and3(d);
    }

    else {
        qDebug() << "Unsupported protocol version" << protocolVersion;
    }
    return outBuffer;
}
