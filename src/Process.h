//
// Created by helidium on 3/22/17.
//

#ifndef MNS_PROCESS_H
#define MNS_PROCESS_H

#include <sched.h>

namespace MNS {
	/**
	 * Helper struct for process affinity
	 */
	struct Process {
		/**
		 * Tie the process to CPU core
		 * @return
		 */
		static int setAffinity() {
#ifdef __linux__
			cpu_set_t mask;
			CPU_ZERO(&mask);
			CPU_SET(1, &mask);

			return sched_setaffinity(0, sizeof(mask), &mask);
#else
			return 0;
#endif
		}
	};
}

#endif //MNS_PROCESS_H
