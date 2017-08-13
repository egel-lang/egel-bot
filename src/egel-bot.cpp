#include <sys/types.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/socket.h>
#include <netdb.h>
#include <string.h>
#include <vector>
#include <functional>

#include "utils.hpp"
#include "position.hpp"
#include "reader.hpp"
#include "lexical.hpp"
#include "syntactical.hpp"
#include "machine.hpp"
#include "modules.hpp"
#include "eval.hpp"
#include "builtin.hpp"

#define EXECUTABLE_NAME "egel-bot"

#define EXECUTABLE_VERSION_MAJOR   "0"
#define EXECUTABLE_VERSION_MINOR   "0"
#define EXECUTABLE_VERSION_RELEASE "1"

#define EXECUTABLE_VERSION \
    EXECUTABLE_VERSION_MAJOR "." \
    EXECUTABLE_VERSION_MINOR "." \
    EXECUTABLE_VERSION_RELEASE

#define EXECUTABLE_COPYRIGHT \
    "Copyright (C) 2017"
#define EXECUTABLE_AUTHORS \
    "M.C.A. (Marco) Devillers"

#define BUFFER_SIZE 512

class IRCChannel {
public:
    IRCChannel(): _fd(-1) {
    }

    void link(const UnicodeString& node, const UnicodeString& service) {
        // code adopted from the getaddrinfo man page
        struct addrinfo hints;
        struct addrinfo *result, *rp;
        int sfd, s;

        /* Obtain address(es) matching host/port */
        memset(&hints, 0, sizeof(struct addrinfo));
        hints.ai_family = AF_UNSPEC;     /* Allow IPv4 or IPv6 */
        hints.ai_socktype = SOCK_STREAM; /* stream socket */
        hints.ai_flags = AI_PASSIVE;
        hints.ai_protocol = 0;           /* Any protocol */

        auto nod = unicode_to_char(node);
        auto srv = unicode_to_char(service);
        s = getaddrinfo(nod, srv, &hints, &result);
        delete[] nod;
        delete[] srv;

        if (s != 0) {
           fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(s));
           exit(EXIT_FAILURE);
        }

        /* getaddrinfo() returns a list of address structures.
          Try each address until we successfully connect(2).
          If socket(2) (or connect(2)) fails, we (close the socket
          and) try the next address. */
        for (rp = result; rp != NULL; rp = rp->ai_next) {
           sfd = socket(rp->ai_family, rp->ai_socktype,
                        rp->ai_protocol);
           if (sfd == -1)
               continue;

           if (connect(sfd, rp->ai_addr, rp->ai_addrlen) != -1)
               break;                  /* Success */

           close(sfd);
        }

        if (rp == NULL) {               /* No address succeeded */
           fprintf(stderr, "Could not connect\n");
           exit(EXIT_FAILURE);
        }

        freeaddrinfo(result);           /* No longer needed */

        std::cout << "connected to " << node << " " << service << std::endl;
        _fd = sfd;
    }

    void unlink() {
        close(_fd);
    }

    char read() {
        char buffer[1];
        recv(_fd, buffer, 1, 0);
        return buffer[0];
    }

    UnicodeString in() {
        // read chars until '\r\n' is received
        char buffer[BUFFER_SIZE];
        int n = 0;
        while (n < BUFFER_SIZE-2) {
            char c = read();
            if (c == '\r') {
                c = read();
                if (c == '\n') {
                    buffer[n] = '\0';
                    n++;
                    goto flush;
                } else {
                    buffer[n] = '\r';
                    n++;
                    buffer[n] = c;
                    n++;
                }
            } else {
                buffer[n] = c;
                n++;
            }
        }

        std::cerr << "warning: garbled input" << std::endl;
        return UnicodeString("");

        flush:
        return UnicodeString(buffer);
    }

    void out(const UnicodeString& o) {
        UnicodeString s = o + "\r\n";
        auto buf = unicode_to_char(s);
        int len = strlen(buf);
        if (len != send(_fd,buf,len,0)) {
            std::cerr << "warning: garbled output" << std::endl;
        }
        delete[] buf;
    }

private:
    int _fd;
};

// System.say o
// send a PRIVMSG to the channel
class Say: public Monadic {
public:
    MONADIC_PREAMBLE(Say, "System", "say");

    typedef std::function<void(const UnicodeString&)> say_handler_t;

    void set_handler(say_handler_t h) {
        _say_handler = h;
    }

    void say(const UnicodeString& msg) const {
        UnicodeString m = msg;
        m = m.findAndReplace("\n","-");
        _say_handler(m);
    }

    VMObjectPtr apply(const VMObjectPtr& arg0) const override {

        static VMObjectPtr nop = nullptr;
        if (nop == nullptr) nop = machine()->get_data_string("System", "nop");

        if (arg0->tag() == VM_OBJECT_INTEGER) {
            say(arg0->to_text());
            return nop;
        } else if (arg0->tag() == VM_OBJECT_FLOAT) {
            say(arg0->to_text());
            return nop;
        } else if (arg0->tag() == VM_OBJECT_CHAR) {
            say(arg0->to_text());
            return nop;
        } else if (arg0->tag() == VM_OBJECT_TEXT) {
            auto s = VM_OBJECT_TEXT_VALUE(arg0);
            say(s);
            return nop;
        } else {
            return nullptr;
        }
    }

private:
    say_handler_t  _say_handler;
};

class IRCHandler {
public:
    IRCHandler(const UnicodeString& node, const UnicodeString& service, const UnicodeString& channel, const UnicodeString& nick):
        _node(node), _service(service), _channel(channel), _nick(nick) {
    }

private:
    UnicodeString _node;
    UnicodeString _service;
    UnicodeString _channel;
    UnicodeString _nick;

    bool _stop = false;
    IRCChannel _chan;

    VM*     _machine;
    Eval*   _eval;

public:
    void init() {
        // create the connection
        link(_node, _service);

        // set nick and user, join the channel
        out("NICK " + _nick);
        out("USER " + _nick + " localhost 0 : egel language IRC bot");

        // start up the module system
        OptionsPtr oo = Options().clone();
        ModuleManagerPtr mm = ModuleManager().clone();
        _machine = new Machine();
        NamespacePtr env = Namespace().clone();
        mm->init(oo, _machine, env);

        // bypass module loading and directly insert local combinator into machine.
        auto say = (std::static_pointer_cast<Say>) (Say(_machine).clone());
        say->set_handler(
                std::bind( &IRCHandler::out_message, this, std::placeholders::_1)
            );

        mm->declare_object(say);

        // fire up the evaluator
        _eval = new Eval(mm);

        try {
            _eval->eval_load("script.eg");
            _eval->eval_command("using System");
        } catch (Error e) {
            std::cerr << e << std::endl;
            exit(EXIT_FAILURE);
        }
    }

    void link(const UnicodeString& node, const UnicodeString& service) {
        _chan.link(_node, _service);
    }

    UnicodeString in() {
        auto s = _chan.in();
        if (s != "") std::cout << "recv: " << s << std::endl;
        return s;
    }

    void out(const UnicodeString& s) {
        std::cout << "send: " << s << std::endl;
        _chan.out(s);
    }

    void out_message(const UnicodeString& s) {
        out("PRIVMSG #" + _channel + " :" + s);
    }

    void out_quit(const UnicodeString& reason) {
        out("QUIT :" + reason);
    }

    void out_error(const UnicodeString& e) {
        out_message(e);
    }

    UnicodeString remove_source(UnicodeString& s) {
        if (s.startsWith(":")) {
            int n = s.indexOf(" ");
            return s.remove(0,n+1);
        } else {
            return s;
        }
    }

    UnicodeString remove_command(const UnicodeString& command, UnicodeString& s) {
        return s.remove(0, command.length()+1);
    }

    void process() {
        UnicodeString s;
        while (true) {
            s = in();
            s = remove_source(s);
            for (auto d:_dispatch_table) {
                if (s.startsWith(d.first)) {
                    s = remove_command(d.first, s);
                    std::cout << d.first << " " << s << std::endl;
                    (this->*(d.second))(s);
                }
            }
        }
    }
    
    void process_welcome(const UnicodeString& in) {
    }

    void process_MOTD(const UnicodeString& in) {
    }

    void process_MOTD_end(const UnicodeString& in) {
        out("JOIN #" + _channel);
    }

    void process_name_reply(const UnicodeString& in) {
    }

    void process_name_reply_end(const UnicodeString& in) {
    }

    void process_topic(const UnicodeString& in) {
    }

    void process_ping(const UnicodeString& in) {
        UnicodeString s = "PONG " + in;
        out (s);
    }

    void process_nick(const UnicodeString& in) {
    }

    void  process_join(const UnicodeString& in) {
    }

    void process_part(const UnicodeString& in) {
    }

    void process_quit(const UnicodeString& in) {
    }

    UnicodeString remove_colon(UnicodeString s) {
        int n = s.indexOf(":");
        return s.remove(0,n+1);
    }

    UnicodeString result(const VMObjectPtr& o) {
        return o->to_text();
    }

    void main_callback(VM* vm, const VMObjectPtr& o) {
        symbol_t nop = vm->enter_symbol("System", "nop");
        if (o->symbol() != nop) {
            out_message(result(o));
        }
    }

    void exception_callback(VM* vm, const VMObjectPtr& e) {
         out_message("exception(" + result(e) + ")");
    }

    void process_message(const UnicodeString& in) {
        UnicodeString s = in;
        callback_t main = std::bind( &IRCHandler::main_callback, this, std::placeholders::_1, std::placeholders::_2 );
        callback_t exc  = std::bind( &IRCHandler::exception_callback, this, std::placeholders::_1, std::placeholders::_2 );
        if (s.startsWith("#" + _channel)) {
            s = remove_colon(s);
            if (s.startsWith(_nick + ":")) {
                s = remove_colon(s);
                try {
                    _eval->eval_line(s, main, exc);
                } catch (Error e) {
                    out_error(e.error());
                }
            }
        }
    }

    void process_notice(const UnicodeString& in) {    
    }

    void process_mode(const UnicodeString& in) {
    }

    void process_topic_changed(const UnicodeString& in) {
    }

    void process_kick(const UnicodeString& in) {
    }

private:
    typedef void (IRCHandler::*method_ptr)(const UnicodeString& in);
    typedef std::map<UnicodeString, method_ptr> dispatch_t;

    const dispatch_t _dispatch_table = {
        {"001", &IRCHandler::process_welcome},
        {"RPL_WELCOME", &IRCHandler::process_welcome},

        {"375", &IRCHandler::process_MOTD},
        {"RPL_MOTDSTART", &IRCHandler::process_MOTD},
        {"372", &IRCHandler::process_MOTD},
        {"RPL_MOTD", &IRCHandler::process_MOTD},
        {"376", &IRCHandler::process_MOTD_end},
        {"RPL_ENDOFMOTD", &IRCHandler::process_MOTD_end},

        {"PING", &IRCHandler::process_ping},

        {"NICK", &IRCHandler::process_nick},

        {"353", &IRCHandler::process_name_reply},
        {"RPL_NAMREPLY", &IRCHandler::process_name_reply},
        {"366", &IRCHandler::process_name_reply_end},
        {"RPL_ENDOFNAMES", &IRCHandler::process_name_reply_end},
        {"331", &IRCHandler::process_topic},
        {"RPL_NOTOPIC", &IRCHandler::process_topic},
        {"332", &IRCHandler::process_topic},
        {"RPL_TOPIC", &IRCHandler::process_topic},

        {"JOIN", &IRCHandler::process_join},
        {"PART", &IRCHandler::process_part},
        {"QUIT", &IRCHandler::process_quit},

        {"PRIVMSG", &IRCHandler::process_message},
        {"NOTICE", &IRCHandler::process_notice},

        {"MODE", &IRCHandler::process_mode},
        {"TOPIC", &IRCHandler::process_topic_changed},
        {"KICK", &IRCHandler::process_kick}
    };

};

void display_usage () {
    std::cout << "Usage: " << EXECUTABLE_NAME << " node service channel nick" << std::endl;
    /*
    std::cout << EXECUTABLE_NAME << ' ' << EXECUTABLE_VERSION << std::endl;
    std::cout << EXECUTABLE_COPYRIGHT << ' ' << EXECUTABLE_AUTHORS << std::endl;
    */
}

int main(int argc, char *argv[]) {
    if (argc != 5) {
        display_usage();
        return EXIT_FAILURE;
    }

    std::vector<UnicodeString> aa;
    aa.push_back(UnicodeString(argv[1]));
    aa.push_back(UnicodeString(argv[2]));
    aa.push_back(UnicodeString(argv[3]));
    aa.push_back(UnicodeString(argv[4]));

    IRCHandler handler(aa[0], aa[1], aa[2], aa[3]);
    handler.init();
    handler.process();

    return EXIT_SUCCESS;
}
