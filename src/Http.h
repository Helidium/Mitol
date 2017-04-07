//
// Created by helidium on 3/22/17.
//

#ifndef MNS_HTTP_H
#define MNS_HTTP_H

#include "Server.h"

namespace MNS {
    /**
     * Wrapper class for the Server object
     */
    class Http {
    public:
	    /// Default constructor
        Http();

	    /**
	     * Created the Server instance
	     * @return Server instance
	     */
        MNS::Server *createServer();

	    /// Default destructor
        ~Http();
    private:
	    /// Server instance
        MNS::Server *server;
    protected:
    };
}


#endif //MNS_HTTP_H
