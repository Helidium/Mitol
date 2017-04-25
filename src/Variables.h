//
// Created by helidium on 3/24/17.
//

#ifndef MNS_VARIABLES_H
#define MNS_VARIABLES_H

#include <map>
#include <string>

namespace MNS {
    /// HTTP_VERSION
    enum HTTP_VERSION {
	    UNKNOWN_VERSION,
        HTTP_1_0,
        HTTP_1_1
    };

	/// HTTP_METHOD
    enum HTTP_METHOD {
	    UNKNOWN_METHOD,
        OPTIONS,
        GET,
        HEAD,
        POST,
        PUT,
        DELETE,
        TRACE,
        CONNECT,
        PATCH
    };
}

#endif //MNS_VARIABLES_H
