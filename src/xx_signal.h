#ifdef __linux__
#include <csignal>
#endif

namespace xx {
	// 忽略某信号量
	// linux 下在 main() 一开始执行, 可解决 socket 发送时的 broken pipe 问题
#ifdef __linux__
	inline int IgnoreSignal(int const& sig = SIGPIPE) {
		struct sigaction sa;
		sa.sa_handler = SIG_IGN;
		sa.sa_flags = 0;
		if (sigemptyset(&sa.sa_mask) == -1 || sigaction(sig, &sa, 0) == -1) return -1;
		signal(sig, SIG_IGN);
		sigset_t signal_mask;
		sigemptyset(&signal_mask);
		sigaddset(&signal_mask, sig);
		if (int r = pthread_sigmask(SIG_BLOCK, &signal_mask, nullptr)) return r;
		return sigprocmask(SIG_BLOCK, &signal_mask, nullptr);
	}
#else
	inline int IgnoreSignal(int const& sig = 0) { return 0; }
#endif
}
