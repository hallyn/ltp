 * with this program; if not, write the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
#include <aio.h>		/* for aio_read,write */
#include <inttypes.h>		/* for uint64_t type */
#include <siginfo.h>		/* signal handlers & SA_SIGINFO */
#include <sys/uio.h>		/* for struct iovec (readv) */
#include <sys/mman.h>		/* for mmap(2) */
#include <sys/ipc.h>		/* for i/o buffer in shared memory */
#include <sys/shm.h>		/* for i/o buffer in shared memory */
#include <sys/time.h>		/* for delays */
#define	MEM_DATA	1	/* data space                           */
#define	MEM_SHMEM	2	/* System V shared memory               */
#define	MEM_T3ESHMEM	3	/* T3E Shared Memory                    */
#define	MEM_MMAP	4	/* mmap(2)                              */
#define	MEMF_FILE	01000	/* regular file -- unlink on close      */
	int memtype;
	int flags;
	int nblks;
	char *name;
	void *space;		/* memory address of allocated space */
	int fd;			/* FD open for mmaping */
	int size;
} Memalloc[NMEMALLOC];
	char c_file[MAX_FNAME_LENGTH + 1];
	int c_oflags;
	int c_fd;
	long c_rtc;
	int c_memalign;		/* from F_DIOINFO */
	int c_miniosz;
	int c_maxiosz;
	void *c_memaddr;	/* mmapped address */
	int c_memlen;		/* length of above region */
	char *string;
	int value;
	int busy;
	int id;
	int fd;
	int strategy;
	volatile int done;
	struct iosw iosw;
	aiocb_t aiocb;
	int aio_ret;		/* from aio_return */
	int aio_errno;		/* from aio_error */
	int sig;
	int signalled;
	struct sigaction osa;
struct status {
	int rval;		/* syscall return */
	int err;		/* errno */
	int *aioid;		/* list of async I/O structures */
	char *sy_name;
	int sy_type;
	struct status *(*sy_syscall) ();
	int (*sy_buffer) ();
	char *(*sy_format) ();
	int sy_flags;
	int sy_bits;
#define O_SSD 0			/* so code compiles on a CRAY2 */
#define O_PARALLEL 0		/* so O_PARALLEL may be used in expressions */
#define PPID_CHECK_INTERVAL 5	/* check ppid every <-- iterations */
#define	MAX_AIO		256	/* maximum number of async I/O ops */
#define	MPP_BUMP	16	/* page un-alignment for MPP */
int a_opt = 0;			/* abort on data compare errors     */
int e_opt = 0;			/* exec() after fork()'ing          */
int C_opt = 0;			/* Data Check Type                  */
int d_opt = 0;			/* delay between operations         */
int k_opt = 0;			/* lock file regions during writes  */
int m_opt = 0;			/* generate periodic messages       */
int n_opt = 0;			/* nprocs                           */
int r_opt = 0;			/* resource release interval        */
int w_opt = 0;			/* file write log file              */
int v_opt = 0;			/* verify writes if set             */
int U_opt = 0;			/* upanic() on varios conditions    */
int V_opt = 0;			/* over-ride default validation fd type */
int M_opt = 0;			/* data buffer allocation types     */
char TagName[40];		/* name of this doio (see Monster)  */
char *Prog = NULL;		/* set up in parse_cmdline()                */
int Upanic_Conditions;		/* set by args to -U                        */
int Release_Interval;		/* arg to -r                                */
int Nprocs;			/* arg to -n                                */
char *Write_Log;		/* arg to -w                                */
char *Infile;			/* input file (defaults to stdin)           */
int *Children;			/* pids of child procs                      */
int Nchildren = 0;
int Nsiblings = 0;		/* tfork'ed siblings                        */
int Execd = 0;
int Message_Interval = 0;
int Npes = 0;			/* non-zero if built as an mpp multi-pe app */
int Vpe = -1;			/* Virtual pe number if Npes >= 0           */
int Reqno = 1;			/* request # - used in some error messages  */
int Reqskipcnt = 0;		/* count of I/O requests that are skipped   */
int Validation_Flags;
char *(*Data_Check) ();		/* function to call for data checking       */
int (*Data_Fill) ();		/* function to call for data filling        */
int Nmemalloc = 0;		/* number of memory allocation strategies   */
int delayop = 0;		/* delay between operations - type of delay */
int delaytime = 0;		/* delay between operations - how long      */

struct wlog_file Wlog;

int active_mmap_rw = 0;		/* Indicates that mmapped I/O is occurring. */
int havesigint = 0;
int Wfd_Append;			/* for appending to the write-log       */
int Wfd_Random;			/* for overlaying write-log entries     */
#define FD_ALLOC_INCR	32	/* allocate this many fd_map structs    */
char *Memptr;			/* ptr to core buffer space             */
int Memsize;			/* # bytes pointed to by Memptr         */
				/* maintained by alloc_mem()            */
int Sdsptr;			/* sds offset (always 0)                */
int Sdssize;			/* # bytes of allocated sds space       */
				/* Maintained by alloc_sds()            */
char Host[16];
char Pattern[128];
int Pattern_Length;
char *syserrno(int err);
void doio(void);
void doio_delay(void);
char *format_oflags(int oflags);
char *format_strat(int strategy);
char *format_rw(struct io_req *ioreq, int fd, void *buffer,
		int signo, char *pattern, void *iosw);
char *format_sds(struct io_req *ioreq, void *buffer, int sds char *pattern);
int do_read(struct io_req *req);
int do_write(struct io_req *req);
int lock_file_region(char *fname, int fd, int type, int start, int nbytes);
char *format_listio(struct io_req *ioreq, int lcmd,
		    struct listreq *list, int nent, int fd, char *pattern);
int do_listio(struct io_req *req);
int do_ssdio(struct io_req *req);
char *fmt_ioreq(struct io_req *ioreq, struct syscall_info *sy, int fd);
struct status *sy_listio(struct io_req *req, struct syscall_info *sysc,
			 int fd, char *addr);
int listio_mem(struct io_req *req, int offset, int fmstride,
	       int *min, int *max);
char *fmt_listio(struct io_req *req, struct syscall_info *sy,
		 int fd, char *addr);
struct status *sy_pread(struct io_req *req, struct syscall_info *sysc,
struct status *sy_pwrite(struct io_req *req, struct syscall_info *sysc,
			 int fd, char *addr);
char *fmt_pread(struct io_req *req, struct syscall_info *sy,
		int fd, char *addr);
#endif /* sgi */
struct status *sy_readv(struct io_req *req, struct syscall_info *sysc,
struct status *sy_writev(struct io_req *req, struct syscall_info *sysc,
			 int fd, char *addr);
struct status *sy_rwv(struct io_req *req, struct syscall_info *sysc,
		      int fd, char *addr, int rw);
char *fmt_readv(struct io_req *req, struct syscall_info *sy,
		int fd, char *addr);
struct status *sy_aread(struct io_req *req, struct syscall_info *sysc,
struct status *sy_awrite(struct io_req *req, struct syscall_info *sysc,
			 int fd, char *addr)
struct status *sy_arw(struct io_req *req, struct syscall_info *sysc,
		      int fd, char *addr, int rw);
char *fmt_aread(struct io_req *req, struct syscall_info *sy,
		int fd, char *addr);
struct status *sy_mmread(struct io_req *req, struct syscall_info *sysc,
			 int fd, char *addr);
struct status *sy_mmwrite(struct io_req *req, struct syscall_info *sysc,
			  int fd, char *addr);
struct status *sy_mmrw(struct io_req *req, struct syscall_info *sysc,
		       int fd, char *addr, int rw);
char *fmt_mmrw(struct io_req *req, struct syscall_info *sy, int fd, char *addr);
int do_rw(struct io_req *req);
int do_fcntl(struct io_req *req);
int do_sync(struct io_req *req);
int doio_pat_fill(char *addr, int mem_needed, char *Pattern,
		  int Pattern_Length, int shift);
char *doio_pat_check(char *buf, int offset, int length,
		     char *pattern, int pattern_length, int patshift);
char *check_file(char *file, int offset, int length, char *pattern,
		 int pattern_length, int patshift, int fsa);
int doio_fprintf(FILE * stream, char *format, ...);
int alloc_mem(int nbytes);
int alloc_sds(int nbytes);
int alloc_fd(char *file, int oflags);
struct fd_cache *alloc_fdcache(char *file, int oflags);
void signal_info(int sig, siginfo_t * info, void *v);
void cleanup_handler(int sig, siginfo_t * info, void *v);
void die_handler(int sig, siginfo_t * info, void *v);
void sigbus_handler(int sig, siginfo_t * info, void *v);
#else /* !sgi */
void cleanup_handler(int sig);
void die_handler(int sig);
void sigbus_handler(int sig);
void noop_handler(int sig);
void sigint_handler(int sig);
void aio_handler(int sig);
void dump_aio(void);
void cb_handler(sigval_t val);
struct aio_info *aio_slot(int aio_id);
int aio_register(int fd, int strategy, int sig);
int aio_unregister(int aio_id);
int aio_wait(int aio_id);
char *hms(time_t t);
int aio_done(struct aio_info *ainfo);
void doio_upanic(int mask);
int parse_cmdline(int argc, char **argv, char *opts);
void parse_memalloc(char *arg);
void dump_memalloc(void);
void parse_delay(char *arg);
int usage(FILE * stream);
void help(FILE * stream);
#define U_CORRUPTION	0001	/* upanic on data corruption    */
#define U_IOSW	    	0002	/* upanic on bad iosw           */
#define U_RVAL	    	0004	/* upanic on bad rval           */
	{"corruption", U_CORRUPTION},
	{"iosw", U_IOSW},
	{"rval", U_RVAL},
	{"all", U_ALL},
	{NULL, 0}
struct aio_info Aio_Info[MAX_AIO];
	{"default", C_DEFAULT},
	{NULL, 0},
#define	DELAY_ITIMER	5	/* POSIX timer                          */
	{"select", DELAY_SELECT},
	{"sleep", DELAY_SLEEP},
	{"sginap", DELAY_SGINAP},
	{"alarm", DELAY_ALARM},
	{NULL, 0},
int main(int argc, char **argv)
	int i, pid, stat, ex_stat;
	sigset_t omask;
	sigset_t omask, block_mask;
	int omask;
	struct sigaction sa;
	random_range_seed(getpid());	/* initialize random number generator */
	/* first time */
		switch (i) {
					     i + 1, SYSERR, errno);
					argv[0] =
					    (char *)
					    malloc(strlen(exec_path + 1));
			} else if (WIFSIGNALED(stat)
				   && WTERMSIG(stat) != SIGINT) {
}				/* main */
void doio(void)
	int rval, i, infd, nbytes;
	char *cp;
	struct io_req ioreq;
	struct sigaction sa, def_action, ignore_action, exit_action;
	struct sigaction sigbus_action;
		switch (i) {
			/* Signals to Ignore... */
			/* Signals to trap & report & die */
			/*case SIGTRAP: */
			/*case SIGABRT: */
#ifdef SIGERR			/* cray only signals */
			/*case SIGFPE: */
			/* Default Action for all other signals */
		if (Reqno && Release_Interval && !(Reqno % Release_Interval)) {
		} else if (rval != 0) {
			doio_fprintf(stderr,
				     "Info:  %d requests done (%d skipped) by this process\n",
				     Reqno, Reqskipcnt);
}				/* doio */
void doio_delay(void)
	switch (delayop) {
		   tv_delay.tv_sec, tv_delay.tv_usec); */
	{"READ", READ},
	{"WRITE", WRITE},
	{"READA", READA},
	{"WRITEA", WRITEA},
	{"SSREAD", SSREAD},
	{"SSWRITE", SSWRITE},
	{"LISTIO", LISTIO},
	{"LREAD", LREAD},
	{"LREADA", LREADA},
	{"LWRITE", LWRITE},
	{"LWRITEA", LWRITEA},
	{"LSREAD", LSREAD},
	{"LSREADA", LSREADA},
	{"LSWRITE", LSWRITE},
	{"LSWRITEA", LSWRITEA},
	{"PREAD", PREAD},
	{"PWRITE", PWRITE},
	{"AREAD", AREAD},
	{"AWRITE", AWRITE},
	{"LLREAD", LLREAD},
	{"LLAREAD", LLAREAD},
	{"LLWRITE", LLWRITE},
	{"LLAWRITE", LLAWRITE},
	{"RESVSP", RESVSP},
	{"UNRESVSP", UNRESVSP},
	{"DFFSYNC", DFFSYNC},
	{"READV", READV},
	{"WRITEV", WRITEV},
	{"MMAPR", MMAPR},
	{"MMAPW", MMAPW},
	{"FSYNC2", FSYNC2},
	{"FDATASYNC", FDATASYNC},

	{"unknown", -1},
	{"poll", A_POLL},
	{"signal", A_SIGNAL},
	{"recall", A_RECALL},
	{"recalla", A_RECALLA},
	{"recalls", A_RECALLS},
	{"suspend", A_SUSPEND},
	{"callback", A_CALLBACK},
	{"synch", 0},
	{"unknown", -1},
char *format_oflags(int oflags)
	flags[0] = '\0';
	switch (oflags & 03) {
	case O_RDONLY:
		strcat(flags, "O_RDONLY,");
		break;
	case O_WRONLY:
		strcat(flags, "O_WRONLY,");
		break;
	case O_RDWR:
		strcat(flags, "O_RDWR,");
		break;
	default:
		strcat(flags, "O_weird");
		break;
		strcat(flags, "O_EXCL,");
		strcat(flags, "O_SYNC,");
		strcat(flags, "O_RAW,");
		strcat(flags, "O_WELLFORMED,");
		strcat(flags, "O_SSD,");
		strcat(flags, "O_LDRAW,");
		strcat(flags, "O_PARALLEL,");
		strcat(flags, "O_BIG,");
		strcat(flags, "O_PLACE,");
		strcat(flags, "O_ASYNC,");
		strcat(flags, "O_DIRECT,");
		strcat(flags, "O_DSYNC,");
		strcat(flags, "O_RSYNC,");
	return (strdup(flags));
char *format_strat(int strategy)
	case A_POLL:
		aio_strat = "POLL";
		break;
	case A_SIGNAL:
		aio_strat = "SIGNAL";
		break;
	case A_RECALL:
		aio_strat = "RECALL";
		break;
	case A_RECALLA:
		aio_strat = "RECALLA";
		break;
	case A_RECALLS:
		aio_strat = "RECALLS";
		break;
	case A_SUSPEND:
		aio_strat = "SUSPEND";
		break;
	case A_CALLBACK:
		aio_strat = "CALLBACK";
		break;
	case 0:
		aio_strat = "<zero>";
		break;
	return (aio_strat);
char *format_rw(struct io_req *ioreq, int fd, void *buffer, int signo,
		char *pattern, void *iosw)
	static char *errbuf = NULL;
	char *aio_strat, *cp;
	struct read_req *readp = &ioreq->r_data.read;
	struct write_req *writep = &ioreq->r_data.write;
	struct read_req *readap = &ioreq->r_data.read;
	struct write_req *writeap = &ioreq->r_data.write;
			      fd, (unsigned long)buffer, readp->r_nbytes);
		cp +=
		    sprintf(cp,
			    "          fd %d is file %s - open flags are %#o\n",
			    fd, readp->r_file, readp->r_oflags);
		cp +=
		    sprintf(cp, "          read done at file offset %d\n",
			    readp->r_offset);
			      fd, (unsigned long)buffer, writep->r_nbytes);
		cp +=
		    sprintf(cp,
			    "          fd %d is file %s - open flags are %#o\n",
			    fd, writep->r_file, writep->r_oflags);
		cp +=
		    sprintf(cp,
			    "          write done at file offset %d - pattern is %s\n",
			    writep->r_offset, pattern);
			      fd, (unsigned long)buffer, readap->r_nbytes,
			      (unsigned long)iosw, signo);
		cp +=
		    sprintf(cp,
			    "          fd %d is file %s - open flags are %#o\n",
			    fd, readap->r_file, readp->r_oflags);
		cp +=
		    sprintf(cp, "          reada done at file offset %d\n",
			    readap->r_offset);
		cp +=
		    sprintf(cp,
			    "          async io completion strategy is %s\n",
			    aio_strat);
			      fd, (unsigned long)buffer, writeap->r_nbytes,
			      (unsigned long)iosw, signo);
		cp +=
		    sprintf(cp,
			    "          fd %d is file %s - open flags are %#o\n",
			    fd, writeap->r_file, writeap->r_oflags);
		cp +=
		    sprintf(cp,
			    "          writea done at file offset %d - pattern is %s\n",
			    writeap->r_offset, pattern);
		cp +=
		    sprintf(cp,
			    "          async io completion strategy is %s\n",
			    aio_strat);
char *format_sds(struct io_req *ioreq, void *buffer, int sds, char *pattern)
	int i;
	static char *errbuf = NULL;
	char *cp;
	struct ssread_req *ssreadp = &ioreq->r_data.ssread;
	struct sswrite_req *sswritep = &ioreq->r_data.sswrite;
		cp +=
		    sprintf(cp,
			    "syscall:  sswrite(%#o, %#o, %d) - pattern was %s\n",
			    buffer, sds, sswritep->r_nbytes, pattern);
int do_read(struct io_req *req)
	int fd, offset, nbytes, oflags, rval;
	char *addr, *file;
	struct aio_info *aiop;
	int aio_id, aio_strat, signo;
	struct fd_cache *fdc;
	/*printf("read: %s, %#o, %d %d\n", file, oflags, offset, nbytes); */
	 *      information is available in mem. allocate
		if ((rval =
		     alloc_mem(nbytes + wtob(1) * 2 +
			       MPP_BUMP * sizeof(UINT64_T))) < 0) {
		if (!(req->r_data.read.r_uflags & F_WORD_ALIGNED)) {
			addr +=
			    fdc->c_memalign - ((long)addr % fdc->c_memalign);
#endif /* !CRAY && sgi */
#endif /* CRAY */
		/* move to the desired file position. */
		/* move to the desired file position. */
				     format_rw(req, fd, addr, signo, NULL,
					       &aiop->iosw));
					     format_rw(req, fd, addr, signo,
						       NULL, &aiop->iosw));
#endif /* CRAY */
int do_write(struct io_req *req)
	static int pid = -1;
	int fd, nbytes, oflags, signo;
	int logged_write, rval, got_lock;
	off_t offset, woffset;
	char *addr, pattern, *file, *msg;
	struct wlog_rec wrec;
	int aio_strat, aio_id;
	struct aio_info *aiop;
	struct fd_cache *fdc;
	signo = 0;
	nbytes = req->r_data.write.r_nbytes;
	offset = req->r_data.write.r_offset;
	pattern = req->r_data.write.r_pattern;
	file = req->r_data.write.r_file;
	oflags = req->r_data.write.r_oflags;
	/*printf("pwrite: %s, %#o, %d %d\n", file, oflags, offset, nbytes); */
	   fd, file, oflags, offset, nbytes); */
		(*Data_Fill) (Memptr, nbytes, Pattern, Pattern_Length, 0);
		/*pattern_fill(Memptr, nbytes, Pattern, Pattern_Length, 0); */
			doio_fprintf(stderr,
				     "sswrite(%d, %d, %d) failed:  %s (%d)\n",
				     (long)Memptr, Sdsptr, btoc(nbytes), SYSERR,
				     errno);
		doio_fprintf(stderr,
			     "Invalid O_SSD flag was generated for MPP system\n");
		if (!(req->r_data.write.r_uflags & F_WORD_ALIGNED)) {
		(*Data_Fill) (Memptr, nbytes, Pattern, Pattern_Length, 0);
			memmove(addr, Memptr, nbytes);
			addr +=
			    fdc->c_memalign - ((long)addr % fdc->c_memalign);
	(*Data_Fill) (Memptr, nbytes, Pattern, Pattern_Length, 0);
		memmove(addr, Memptr, nbytes);
	(*Data_Fill) (Memptr, nbytes, Pattern, Pattern_Length, 0);
		memmove(addr, Memptr, nbytes);
	switch (req->r_type) {
				     format_rw(req, fd, addr, -1, Pattern,
					       NULL));
				     fdc->c_miniosz, nbytes % fdc->c_miniosz,
				     oflags, fdc->c_memalign,
				     (long)addr % fdc->c_memalign);
				     offset, nbytes % 4096, oflags);
				     format_rw(req, fd, addr, -1, Pattern,
					       NULL));
				     format_rw(req, fd, addr, -1, Pattern,
					       NULL));
					     format_rw(req, fd, addr, -1,
						       Pattern, &aiop->iosw));
			doio_fprintf(stderr, "%s%s\n", msg,
				     format_rw(req, fd, addr, -1, Pattern,
					       &aiop->iosw)
			    );
	return ((rval == -1) ? -1 : 0);
int lock_file_region(char *fname, int fd, int type, int start, int nbytes)
	struct flock flk;
char *format_listio(struct io_req *ioreq, int lcmd, struct listreq *list,
		    int nent, int fd, char *pattern)
	static char *errbuf = NULL;
	struct listio_req *liop = &ioreq->r_data.listio;
	struct listreq *listreq;
	char *cp, *cmd, *opcode, *aio_strat;
	int i;
	case LC_START:
		cmd = "LC_START";
		break;
	case LC_WAIT:
		cmd = "LC_WAIT";
		break;
	default:
		cmd = "???";
		break;
	cp += sprintf(cp, "syscall:  listio(%s, %#o, %d)\n\n", cmd, list, nent);
		case LO_READ:
			opcode = "LO_READ";
			break;
		case LO_WRITE:
			opcode = "LO_WRITE";
			break;
		default:
			opcode = "???";
			break;
		cp +=
		    sprintf(cp, "          li_drvr =      %#o\n",
			    listreq->li_drvr);
		cp +=
		    sprintf(cp, "          li_flags =     %#o\n",
			    listreq->li_flags);
		cp +=
		    sprintf(cp, "          li_offset =    %d\n",
			    listreq->li_offset);
		cp +=
		    sprintf(cp, "          li_fildes =    %d\n",
			    listreq->li_fildes);
		cp +=
		    sprintf(cp, "          li_buf =       %#o\n",
			    listreq->li_buf);
		cp +=
		    sprintf(cp, "          li_nbyte =     %d\n",
			    listreq->li_nbyte);
		cp +=
		    sprintf(cp, "          li_status =    %#o (%d, %d, %d)\n",
			    listreq->li_status, listreq->li_status->sw_flag,
			    listreq->li_status->sw_error,
			    listreq->li_status->sw_count);
		cp +=
		    sprintf(cp, "          li_signo =     %d\n",
			    listreq->li_signo);
		cp +=
		    sprintf(cp, "          li_nstride =   %d\n",
			    listreq->li_nstride);
		cp +=
		    sprintf(cp, "          li_filstride = %d\n",
			    listreq->li_filstride);
		cp +=
		    sprintf(cp, "          li_memstride = %d\n",
			    listreq->li_memstride);
		cp +=
		    sprintf(cp, "          io completion strategy is %s\n",
			    aio_strat);
int do_listio(struct io_req *req)
	struct listio_req *lio;
	int fd, oflags, signo, nb, i;
	int logged_write, rval, got_lock;
	int aio_strat, aio_id;
	int min_byte, max_byte;
	int mem_needed;
	int foffset, fstride, mstride, nstrides;
	char *moffset;
	long offset, woffset;
	char *addr, *msg;
	sigset_t block_mask, omask;
	struct wlog_rec wrec;
	struct aio_info *aiop;
	struct listreq lio_req;
		doio_fprintf(stderr,
			     "do_listio():  Bogus listio request - abs(filestride) [%d] < nbytes [%d]\n",
	    stride_bounds(0, lio->r_memstride, lio->r_nstrides,
			  lio->r_nbytes, NULL, NULL);
	if (!(lio->r_uflags & F_WORD_ALIGNED)) {
		(*Data_Fill) (Memptr, mem_needed, Pattern, Pattern_Length, 0);
			memmove(addr, Memptr, mem_needed);
				     min_byte, (max_byte - min_byte + 1)) < 0) {
			doio_fprintf(stderr,
				     "stride_bounds(%d, %d, %d, %d, ..., ...) set min_byte to %d, max_byte to %d\n",
			     format_listio(req, lio->r_cmd, &lio_req, 1, fd,
					   Pattern));
			     format_listio(req, lio->r_cmd, &lio_req, 1, fd,
					   Pattern));
		if (mstride > 0 || lio->r_nstrides <= 1) {
					     format_listio(req, lio->r_cmd,
							   &lio_req, 1, fd,
							   Pattern));
				exit(E_COMPARE);
lio_done:
				     min_byte, (max_byte - min_byte + 1)) < 0) {
int do_ssdio(struct io_req *req)
	int nbytes, nb;
	char errbuf[BSIZE];
		/*pattern_fill(Memptr, nbytes, Pattern, Pattern_Length, 0); */
		(*Data_Fill) (Memptr, nbytes, Pattern, Pattern_Length, 0);
		if (pattern_check(Memptr, nbytes, Pattern, Pattern_Length, 0) ==
		    -1) {
int do_ssdio(struct io_req *req)
char *fmt_ioreq(struct io_req *ioreq, struct syscall_info *sy, int fd)
	static char *errbuf = NULL;
	char *cp;
	struct rw_req *io;
	struct smap *aname;
	struct stat sbuf;
	struct dioattr finfo;
	for (aname = aionames;
	     aname->value != -1 && aname->value != io->r_aio_strat; aname++) ;
	cp +=
	    sprintf(cp, "          fd %d is file %s - open flags are %#o %s\n",
		    fd, io->r_file, io->r_oflags, format_oflags(io->r_oflags));
		cp +=
		    sprintf(cp,
			    "          write done at file offset %d - pattern is %c (%#o)\n",
			    io->r_offset,
			    (io->r_pattern == '\0') ? '?' : io->r_pattern,
			    io->r_pattern);
			      io->r_offset);
		cp +=
		    sprintf(cp,
			    "          async io completion strategy is %s\n",
			    aname->string);
	cp +=
	    sprintf(cp,
		    "          number of requests is %d, strides per request is %d\n",
		    io->r_nent, io->r_nstrides);
	cp += sprintf(cp, "          i/o byte count = %d\n", io->r_nbytes);
		      (io->
		       r_uflags & F_WORD_ALIGNED) ? "aligned" : "unaligned");
		cp +=
		    sprintf(cp,
			    "          RAW I/O: offset %% 4096 = %d length %% 4096 = %d\n",
			    io->r_offset % 4096, io->r_nbytes % 4096);
		cp +=
		    sprintf(cp,
			    "          optimal file xfer size: small: %d large: %d\n",
			    sbuf.st_blksize, sbuf.st_oblksize);
		cp +=
		    sprintf(cp, "          cblks %d cbits %#o\n", sbuf.st_cblks,
			    sbuf.st_cbits);
			cp +=
			    sprintf(cp,
				    "          Error %s (%d) getting direct I/O info\n",
				    strerror(errno), errno);
		cp +=
		    sprintf(cp,
			    "          DIRECT I/O: offset %% %d = %d length %% %d = %d\n",
			    finfo.d_miniosz, io->r_offset % finfo.d_miniosz,
			    io->r_nbytes, io->r_nbytes % finfo.d_miniosz);
		cp +=
		    sprintf(cp,
			    "          mem alignment 0x%x xfer size: small: %d large: %d\n",
			    finfo.d_mem, finfo.d_miniosz, finfo.d_maxiosz);
	return (errbuf);
struct status *sy_listio(struct io_req *req, struct syscall_info *sysc, int fd,
			 char *addr)
	int offset, nbytes, nstrides, nents, aio_strat;
	int aio_id, signo, o, i, lc;
	char *a;
	struct listreq *lio_req, *l;
	struct aio_info *aiop;
	struct status *status;
	offset = req->r_data.io.r_offset;
	nbytes = req->r_data.io.r_nbytes;
	nstrides = req->r_data.io.r_nstrides;
	nents = req->r_data.io.r_nent;
			     __FILE__, __LINE__);
	status->aioid = (int *)malloc((nents + 1) * sizeof(int));
			     __FILE__, __LINE__);
			     __FILE__, __LINE__);
	for (l = lio_req, a = addr, o = offset, i = 0;
	     i < nents; l++, a += nbytes, o += nbytes, i++) {
		l->li_opcode = (sysc->sy_flags & SY_WRITE) ? LO_WRITE : LO_READ;
		l->li_offset = o;
		l->li_fildes = fd;
		l->li_buf = a;
		l->li_nbyte = nbytes;
		l->li_status = &aiop->iosw;
		l->li_signo = signo;
		l->li_nstride = nstrides;
		l->li_filstride = 0;
		l->li_memstride = 0;
		l->li_drvr = 0;
		l->li_flags = LF_LSEEK;
	status->aioid[nents] = -1;	/* end sentinel */
	return (status);
int listio_mem(struct io_req *req, int offset, int fmstride, int *min, int *max)
	int i, size;
			     req->r_data.io.r_nstrides * req->r_data.io.r_nent,
	return (size);
char *fmt_listio(struct io_req *req, struct syscall_info *sy, int fd,
		 char *addr)
	static char *errbuf = NULL;
	char *cp;
	char *c, *opcode;
	int i;
				     __FILE__, __LINE__);
			return NULL;
	return (errbuf);
struct status *sy_pread(struct io_req *req, struct syscall_info *sysc, int fd,
			char *addr)
	struct status *status;
	rc = pread(fd, addr, req->r_data.io.r_nbytes, req->r_data.io.r_offset);
			     __FILE__, __LINE__);
	return (status);
struct status *sy_pwrite(struct io_req *req, struct syscall_info *sysc, int fd,
			 char *addr)
	struct status *status;
	rc = pwrite(fd, addr, req->r_data.io.r_nbytes, req->r_data.io.r_offset);
			     __FILE__, __LINE__);
	return (status);
char *fmt_pread(struct io_req *req, struct syscall_info *sy, int fd, char *addr)
	static char *errbuf = NULL;
	char *cp;
				     __FILE__, __LINE__);
	return (errbuf);
#endif /* sgi */
struct status *sy_readv(struct io_req *req, struct syscall_info *sysc, int fd,
			char *addr)
struct status *sy_writev(struct io_req *req, struct syscall_info *sysc, int fd,
			 char *addr)
struct status *sy_rwv(struct io_req *req, struct syscall_info *sysc, int fd,
		      char *addr, int rw)
	struct status *status;
	struct iovec iov[2];
			     __FILE__, __LINE__);
	if ((rc = lseek(fd, req->r_data.io.r_offset, SEEK_SET)) == -1) {
		return (status);
	return (status);
char *fmt_readv(struct io_req *req, struct syscall_info *sy, int fd, char *addr)
	static char errbuf[32768];
	char *cp;
	return (errbuf);
struct status *sy_aread(struct io_req *req, struct syscall_info *sysc, int fd,
			char *addr)
struct status *sy_awrite(struct io_req *req, struct syscall_info *sysc, int fd,
			 char *addr)
struct status *sy_arw(struct io_req *req, struct syscall_info *sysc, int fd,
		      char *addr, int rw)
	struct status *status;
	int rc;
	int aio_id, aio_strat, signo;
	struct aio_info *aiop;
			     __FILE__, __LINE__);
	memset((void *)&aiop->aiocb, 0, sizeof(aiocb_t));
	status->aioid = (int *)malloc(2 * sizeof(int));
			     __FILE__, __LINE__);
	return (status);
char *fmt_aread(struct io_req *req, struct syscall_info *sy, int fd, char *addr)
	static char errbuf[32768];
	char *cp;
	cp += sprintf(cp, "syscall:  %s(&aiop->aiocb)\n", sy->sy_name);
	return (errbuf);
struct status *sy_mmread(struct io_req *req, struct syscall_info *sysc, int fd,
			 char *addr)
struct status *sy_mmwrite(struct io_req *req, struct syscall_info *sysc, int fd,
			  char *addr)
struct status *sy_mmrw(struct io_req *req, struct syscall_info *sysc, int fd,
		       char *addr, int rw)
	struct status *status;
	void *mrc = NULL, *memaddr = NULL;
	struct fd_cache *fdc;
	struct stat sbuf;
	int rc;
			     __FILE__, __LINE__);
			doio_fprintf(stderr, "fstat failed, errno=%d\n", errno);
			return (status);
			   rw ? PROT_WRITE | PROT_READ : PROT_READ,
			   MAP_SHARED, fd, 0);
				     mrc, errno);
			return (status);
	if (v_opt)
		msync(fdc->c_memaddr, (int)sbuf.st_size, MS_SYNC);
	if (v_opt) {
		rc = munmap(mrc, (int)sbuf.st_size);
	}
	return (status);
char *fmt_mmrw(struct io_req *req, struct syscall_info *sy, int fd, char *addr)
	static char errbuf[32768];
	char *cp;
	struct fd_cache *fdc;
	void *memaddr;
		      (unsigned long)fdc->c_memaddr);
		      (unsigned long)memaddr, req->r_data.io.r_nbytes,
		      (unsigned long)addr);
	return (errbuf);
	{"listio-read-sync", LREAD,
	 sy_listio, NULL, fmt_listio,
	 SY_IOSW},
	{"listio-read-strides-sync", LSREAD,
	 sy_listio, listio_mem, fmt_listio,
	 SY_IOSW},
	{"listio-read-reqs-sync", LEREAD,
	 sy_listio, listio_mem, fmt_listio,
	 SY_IOSW},
	{"listio-read-async", LREADA,
	 sy_listio, NULL, fmt_listio,
	 SY_IOSW | SY_ASYNC},
	{"listio-read-strides-async", LSREADA,
	 sy_listio, listio_mem, fmt_listio,
	 SY_IOSW | SY_ASYNC},
	{"listio-read-reqs-async", LEREADA,
	 sy_listio, listio_mem, fmt_listio,
	 SY_IOSW | SY_ASYNC},
	{"listio-write-sync", LWRITE,
	 sy_listio, listio_mem, fmt_listio,
	 SY_IOSW | SY_WRITE},
	{"listio-write-strides-sync", LSWRITE,
	 sy_listio, listio_mem, fmt_listio,
	 SY_IOSW | SY_WRITE},
	{"listio-write-reqs-sync", LEWRITE,
	 sy_listio, listio_mem, fmt_listio,
	 SY_IOSW | SY_WRITE},
	{"listio-write-async", LWRITEA,
	 sy_listio, listio_mem, fmt_listio,
	 SY_IOSW | SY_WRITE | SY_ASYNC},
	{"listio-write-strides-async", LSWRITEA,
	 sy_listio, listio_mem, fmt_listio,
	 SY_IOSW | SY_WRITE | SY_ASYNC},
	{"listio-write-reqs-async", LEWRITEA,
	 sy_listio, listio_mem, fmt_listio,
	 SY_IOSW | SY_WRITE | SY_ASYNC},
	{"aread", AREAD,
	 sy_aread, NULL, fmt_aread,
	 SY_IOSW | SY_ASYNC},
	{"awrite", AWRITE,
	 sy_awrite, NULL, fmt_aread,
	 SY_IOSW | SY_WRITE | SY_ASYNC},
	{"pread", PREAD,
	 sy_pread, NULL, fmt_pread,
	 0},
	{"pwrite", PWRITE,
	 sy_pwrite, NULL, fmt_pread,
	 SY_WRITE},
	{"readv", READV,
	 sy_readv, NULL, fmt_readv,
	 0},
	{"writev", WRITEV,
	 sy_writev, NULL, fmt_readv,
	 SY_WRITE},
	{"mmap-read", MMAPR,
	 sy_mmread, NULL, fmt_mmrw,
	 0},
	{"mmap-write", MMAPW,
	 sy_mmwrite, NULL, fmt_mmrw,
	 SY_WRITE},
	{NULL, 0,
	 0, 0, 0,
	 0},
int do_rw(struct io_req *req)
	static int pid = -1;
	int fd, offset, nbytes, nstrides, nents, oflags;
	int rval, mem_needed, i;
	int logged_write, got_lock, pattern;
	off_t woffset;
	int min_byte, max_byte;
	char *addr, *file, *msg;
	struct status *s;
	struct wlog_rec wrec;
	struct syscall_info *sy;
	struct aio_info *aiop;
	struct iosw *iosw;
	struct fd_cache *fdc;
	file = req->r_data.io.r_file;
	oflags = req->r_data.io.r_oflags;
	offset = req->r_data.io.r_offset;
	nbytes = req->r_data.io.r_nbytes;
	nstrides = req->r_data.io.r_nstrides;
	nents = req->r_data.io.r_nent;
	pattern = req->r_data.io.r_pattern;
		doio_fprintf(stderr,
			     "do_rw: too many list requests, %d.  Maximum is %d\n",
		return (-1);
	for (sy = syscalls; sy->sy_name != NULL && sy->sy_type != req->r_type;
	     sy++) ;
		return (-1);
	 *      information is available in mem. allocate
	 *      1 extra word for possible partial-word address "bump"
	 *      1 extra word for dynamic pattern overrun
	 *      MPP_BUMP extra words for T3E non-hw-aligned memory address.
		mem_needed = (*sy->sy_buffer) (req, 0, 0, NULL, NULL);
	if ((rval =
	     alloc_mem(mem_needed + wtob(1) * 2 +
		       MPP_BUMP * sizeof(UINT64_T))) < 0) {
			/*pattern_fill(Memptr, mem_needed, Pattern, Pattern_Length, 0); */
			(*Data_Fill) (Memptr, nbytes, Pattern, Pattern_Length,
				      0);
			if (sswrite((long)Memptr, Sdsptr, btoc(mem_needed)) ==
			    -1) {
				doio_fprintf(stderr,
					     "sswrite(%d, %d, %d) failed:  %s (%d)\n",
		doio_fprintf(stderr,
			     "Invalid O_SSD flag was generated for MPP system\n");
#else /* CRAY */
		doio_fprintf(stderr,
			     "Invalid O_SSD flag was generated for non-Cray system\n");
#endif /* CRAY */
		if (!(req->r_data.io.r_uflags & F_WORD_ALIGNED)) {
			addr +=
			    random_range(0, MPP_BUMP, 1, NULL) * sizeof(int);
			addr +=
			    fdc->c_memalign - ((long)addr % fdc->c_memalign);
			(*Data_Fill) (Memptr, mem_needed, Pattern,
				      Pattern_Length, 0);
				memmove(addr, Memptr, mem_needed);
			(*sy->sy_buffer) (req, offset, 0, &min_byte, &max_byte);
				     min_byte, (max_byte - min_byte + 1)) < 0) {
			doio_fprintf(stderr,
				     "file lock failed:\n%s\n",
				     fmt_ioreq(req, sy, fd));
			doio_fprintf(stderr,
				     "          buffer(req, %d, 0, 0x%x, 0x%x)\n",
				     offset, min_byte, max_byte);
			alloc_mem(-1);
			exit(E_INTERNAL);
	s = (*sy->sy_syscall) (req, sy, fd, addr);
			     (*sy->sy_format) (req, sy, fd, addr));
		for (i = 0; i < nents; i++) {
			for (i = 0; i < nents; i++) {
			for (i = 0; i < nents; i++) {
					break;	/* >>> error condition? */
						     (*sy->sy_format) (req, sy,
								       fd,
								       addr));
				} else if (iosw->sw_count != nbytes * nstrides) {
						     1, 0, nbytes * nstrides,
						     (*sy->sy_format) (req, sy,
								       fd,
								       addr));
						     (*sy->sy_format) (req, sy,
								       fd,
								       addr));
						     (*sy->sy_format) (req, sy,
								       fd,
								       addr));
					     (*sy->sy_format) (req, sy, fd,
							       addr));
		msg = check_file(file, offset, nbytes * nstrides * nents,
				     (*sy->sy_format) (req, sy, fd, addr));
				     min_byte, (max_byte - min_byte + 1)) < 0) {
int do_fcntl(struct io_req *req)
	int fd, oflags, offset, nbytes;
	int rval, op;
	int got_lock;
	int min_byte, max_byte;
	char *file, *msg;
	struct flock flk;
	file = req->r_data.io.r_file;
	oflags = req->r_data.io.r_oflags;
	offset = req->r_data.io.r_offset;
	nbytes = req->r_data.io.r_nbytes;

	flk.l_type = 0;
	flk.l_whence = SEEK_SET;
	flk.l_start = offset;
	flk.l_len = nbytes;
				     min_byte, (nbytes + 1)) < 0) {
			doio_fprintf(stderr, "file lock failed:\n");
			doio_fprintf(stderr,
				     "          buffer(req, %d, 0, 0x%x, 0x%x)\n",
				     offset, min_byte, max_byte);
			alloc_mem(-1);
			exit(E_INTERNAL);
	case RESVSP:
		op = F_RESVSP;
		msg = "f_resvsp";
		break;
	case UNRESVSP:
		op = F_UNRESVSP;
		msg = "f_unresvsp";
		break;
	case DFFSYNC:
		op = F_FSYNC;
		msg = "f_fsync";
		break;
			     (long long)flk.l_start, (long long)flk.l_len);
				     min_byte, (max_byte - min_byte + 1)) < 0) {
int do_sync(struct io_req *req)
	int fd, oflags;
	int rval;
	char *file;
	file = req->r_data.io.r_file;
	oflags = req->r_data.io.r_oflags;
	switch (req->r_type) {
	      int shift)
char *doio_pat_check(char *buf, int offset, int length, char *pattern,
		     int pattern_length, int patshift)
	static char errbuf[4096];
	int nb, i, pattern_index;
	char *cp, *bufend, *ep;
	char actual[33], expected[33];
		ep +=
		    sprintf(ep,
			    "Corrupt regions follow - unprintable chars are represented as '.'\n");
		ep +=
		    sprintf(ep,
			    "-----------------------------------------------------------------\n");
				if ((unsigned int)nb > sizeof(expected) - 1) {
					nb = sizeof(expected) - 1;
				ep +=
				    sprintf(ep,
					    "corrupt bytes starting at file offset %d\n",
					    offset + (int)(cp - buf));
					expected[i] =
					    pattern[(pattern_index +
						     i) % pattern_length];
					if (!isprint(expected[i])) {
					if (!isprint(actual[i])) {
				ep +=
				    sprintf(ep,
					    "    1st %2d expected bytes:  %s\n",
					    nb, expected);
				ep +=
				    sprintf(ep,
					    "    1st %2d actual bytes:    %s\n",
					    nb, actual);
char *check_file(char *file, int offset, int length, char *pattern,
		 int pattern_length, int patshift, int fsa)
	static char errbuf[4096];
	int fd, nb, flags;
	char *buf, *em, *ep;
			file, flags, format_oflags(flags), SYSERR, errno);
	if ((em =
	     (*Data_Check) (buf, offset, length, pattern, pattern_length,
			    patshift)) != NULL) {
		ep +=
		    sprintf(ep, "check_file(%s, %d, %d, %s, %d, %d) failed\n\n",
			    file, offset, length, pattern, pattern_length,
			    patshift);
		ep +=
		    sprintf(ep, "Comparison fd is %d, with open flags %#o\n",
			    fd, flags);
		return (errbuf);
int doio_fprintf(FILE * stream, char *format, ...)
	static int pid = -1;
	char *date;
	int rval;
	struct flock flk;
	va_list arglist;
	gettimeofday(&ts, NULL);
int alloc_mem(int nbytes)
	char *cp;
	void *addr;
	int me = 0, flags, key, shmid;
	static int mturn = 0;	/* which memory type to use */
	struct memalloc *M;
	char filename[255];
	memset(&shm_ds, 0x00, sizeof(struct shmid_ds));
		for (me = 0; me < Nmemalloc; me++) {
			switch (Memalloc[me].memtype) {
				munmap(Memalloc[me].space, Memalloc[me].size);
				doio_fprintf(stderr,
					     "alloc_mem: HELP! Unknown memory space type %d index %d\n",
		mturn = 0;
	switch (M->memtype) {
					munpin(M->space, M->size);
			if ((cp = malloc(nbytes)) == NULL) {
				doio_fprintf(stderr,
					     "malloc(%d) failed:  %s (%d)\n",
					doio_fprintf(stderr,
						     "mpin(0x%lx, %d) failed:  %s (%d)\n",
						     cp, nbytes, SYSERR, errno);
					unlink(M->name);
			if ((M->fd =
			     open(M->name, O_CREAT | O_RDWR, 0666)) == -1) {
				doio_fprintf(stderr,
					     "alloc_mmap: error %d (%s) opening '%s'\n",
					     errno, SYSERR, M->name);
				return (-1);
			if ((M->space = mmap(addr, M->size,
					     PROT_READ | PROT_WRITE,
				doio_fprintf(stderr,
					     "alloc_mem: mmap error. errno %d (%s)\n\tmmap(addr 0x%x, size %d, read|write 0x%x, mmap flags 0x%x [%#o], fd %d, 0)\n\tfile %s\n",
					     errno, SYSERR, addr, M->size,
					     PROT_READ | PROT_WRITE, flags,
					     M->flags, M->fd, M->name);
					     (flags & MAP_PRIVATE) ? "private "
					     : "",
					     (flags & MAP_LOCAL) ? "local " :
					     "",
					     (flags & MAP_AUTORESRV) ?
					     "autoresrv " : "",
					     (flags & MAP_AUTOGROW) ?
					     "autogrow " : "",
					     (flags & MAP_SHARED) ? "shared" :
					     "");
				return (-1);
				shmdt(M->space);
				shmctl(M->fd, IPC_RMID);
				shmctl(M->fd, IPC_RMID, &shm_ds);
				doio_fprintf(stderr,
					     "MEM_SHMEM: nblks(%d) too small:  nbytes=%d  Msize=%d, skipping this req.\n",
					     M->nblks, nbytes, M->size);
			shmid = shmget(key, M->size, IPC_CREAT | 0666);
				doio_fprintf(stderr,
					     "shmget(0x%x, %d, CREAT) failed: %s (%d)\n",
				return (-1);
				doio_fprintf(stderr,
					     "shmat(0x%x, NULL, SHM_RND) failed: %s (%d)\n",
				return (-1);
					doio_fprintf(stderr,
						     "mpin(0x%lx, %d) failed:  %s (%d)\n",
						     M->space, M->size, SYSERR,
						     errno);
				}
		doio_fprintf(stderr,
			     "alloc_mem: HELP! Unknown memory space type %d index %d\n",
int alloc_mem(int nbytes)
	char *cp;
	int ip;
	static char *malloc_space;
		free(malloc_space);
		if (Memsize != 0)
			free(malloc_space);
		if ((cp = malloc_space = malloc(nbytes)) == NULL) {
			doio_fprintf(stderr, "malloc(%d) failed:  %s (%d)\n",
				     nbytes, SYSERR, errno);
			return -1;
#ifdef _CRAYT3E
		/* T3E requires memory to be aligned on 0x40 word boundaries */
		if (ip & 0x3F != 0) {
			doio_fprintf(stderr,
				     "malloc(%d) = 0x%x(0x%x) not aligned by 0x%x\n",
				     nbytes, cp, ip, ip & 0x3f);

			free(cp);
			if ((cp = malloc_space = malloc(nbytes + 0x40)) == NULL) {
				doio_fprintf(stderr,
					     "malloc(%d) failed:  %s (%d)\n",
					     nbytes, SYSERR, errno);
				return -1;
			}
			ip = (int)cp;
			cp += (0x40 - (ip & 0x3F));
		}
		Memptr = cp;
		Memsize = nbytes;
int alloc_sds(int nbytes)
int alloc_sds(int nbytes)
int alloc_fd(char *file, int oflags)
		return (fdc->c_fd);
		return (-1);
struct fd_cache *alloc_fdcache(char *file, int oflags)
	int fd;
	struct fd_cache *free_slot, *oldest_slot, *cp;
	static int cache_size = 0;
	static struct fd_cache *cache = NULL;
	struct dioattr finfo;
		return 0;
		    cp->c_oflags == oflags && strcmp(cp->c_file, file) == 0) {
		cache =
		    (struct fd_cache *)realloc(cache,
					       sizeof(struct fd_cache) *
					       (FD_ALLOC_INCR + cache_size));
			doio_fprintf(stderr,
				     "Could not malloc() space for fd chace");
		for (cp = &cache[cache_size - FD_ALLOC_INCR];
void signal_info(int sig, siginfo_t * info, void *v)
		switch (info->si_code) {
			doio_fprintf(stderr,
				     "signal_info  si_signo %d si_code = SI_QUEUE\n",
			    (info->si_signo == SIGBUS)) {
				doio_fprintf(stderr,
					     "signal_info  si_signo %d si_errno %d si_code = %d  si_addr=%p  active_mmap_rw=%d havesigint=%d\n",
					     active_mmap_rw, havesigint);
			}
			doio_fprintf(stderr,
				     "signal_info: si_signo %d si_errno %d unknown code %d\n",
void cleanup_handler(int sig, siginfo_t * info, void *v)
	havesigint = 1;		/* in case there's a followup signal */
	/*signal_info(sig, info, v); *//* be quiet on "normal" kill */
void die_handler(int sig, siginfo_t * info, void *v)
void sigbus_handler(int sig, siginfo_t * info, void *v)
		cleanup_handler(sig, info, v);
	} else {
		die_handler(sig, info, v);
void cleanup_handler(int sig)
	havesigint = 1;		/* in case there's a followup signal */
void die_handler(int sig)
void sigbus_handler(int sig)
	 */
void noop_handler(int sig)
void sigint_handler(int sig)
	int i;
void aio_handler(int sig)
	unsigned int i;
	struct aio_info *aiop;
void dump_aio(void)
	unsigned int i, count;
	count = 0;
				Aio_Info[i].sig, Aio_Info[i].signalled);
void cb_handler(sigval_t val)
	struct aio_info *aiop;
	aiop = aio_slot(val.sival_int);
struct aio_info *aio_slot(int aio_id)
	unsigned int i;
	static int id = 1;
	struct aio_info *aiop;
			if (!Aio_Info[i].busy) {
		doio_fprintf(stderr, "aio_slot(%d) not found.  Request %d\n",
int aio_register(int fd, int strategy, int sig)
	struct aio_info *aiop;
	struct sigaction sa;
int aio_unregister(int aio_id)
	struct aio_info *aiop;
int aio_wait(int aio_id)
	long mask[RECALL_SIZEOF];
	sigset_t sigset;
	struct aio_info *aiop;
	struct iosw *ioswlist[1];
	const aiocb_t *aioary[1];
		while (!aio_done(aiop)) ;
		sighold(aiop->sig);
			sighold(aiop->sig);
				     SYSERR, errno);
#endif /* CRAY */
		cnt = 0;
				doio_fprintf(stderr,
					     "aio_suspend failed: %s (%d)\n",
					     SYSERR, errno);
			doio_fprintf(stderr,
				     "aio_wait: callback wait took %d tries\n",
				     cnt);
				     SYSERR, errno);
char *hms(time_t t)
	static char ascii_time[9];
	struct tm *ltime;
int aio_done(struct aio_info *ainfo)
			     SYSERR, errno);
	/*printf("%d aio_done aio_errno=%d\n", getpid(), ainfo->aio_errno); */
			doio_fprintf(stderr,
				     "aio_done: aio_return failed: %s (%d)\n",
				     SYSERR, errno);
	return -1;		/* invalid */
void doio_upanic(int mask)
		doio_fprintf(stderr,
			     "WARNING - Could not set the panic flag - upanic(PA_SET) failed:  %s (%d)\n",
	syssgi(1005);		/* syssgi test panic - DEBUG kernels only */
int parse_cmdline(int argc, char **argv, char *opts)
	int c;
	char cc, *cp = NULL, *tok = NULL;
	extern int opterr;
	extern int optind;
	extern char *optarg;
	struct smap *s;
	char *memargs[NMEMALLOC];
	int nmemargs, ma;
			for (s = checkmap; s->string != NULL; s++)
			if (s->string == NULL && tok != NULL) {
			switch (s->value) {
				fprintf(stderr,
					"%s%s:  Warning - Program is a multi-pe application - exec option is ignored.\n",
					Prog, TagName);
				fprintf(stderr,
					"%s%s:  Illegal -m arg (%s):  Must be an integer >= 0\n",
					Prog, TagName, optarg);
			for (ma = 0; ma < nmemargs; ma++) {
			/*dump_memalloc(); */
			fprintf(stderr,
				"%s%s: Error: -M isn't supported on this platform\n",
				Prog, TagName);
			sprintf(TagName, "(%.39s)", optarg);
				fprintf(stderr,
					"%s%s:  Program has been built as a multi-pe app.  -n1 is the only nprocs value allowed\n",
					Prog, TagName);
				if (sscanf
				    (optarg, "%i%c", &Validation_Flags,
				     &cc) != 1) {
					fprintf(stderr,
						"%s:  Invalid -V argument (%s) - must be a decimal, hex, or octal\n",
						Prog, optarg);
					fprintf(stderr,
						"    number, or one of the following strings:  'sync',\n");
					fprintf(stderr,
						"    'buffered', 'parallel', 'ldraw', or 'raw'\n");
					for (s = Upanic_Args; s->string != NULL;
					     s++)
						fprintf(stderr, "%s ",
							s->string);
	if (!C_opt) {
	if (!U_opt)
	if (!n_opt)
	if (!r_opt)
	if (!M_opt) {
void parse_memalloc(char *arg)
	char *allocargs[NMEMALLOC];
	int nalloc;
	struct memalloc *M;
			     Nmemalloc);
				    ((MEMF_PRIVATE | MEMF_LOCAL) == 0))
			if (M->flags & ((MEMF_PRIVATE | MEMF_LOCAL) == 0))
			     allocargs[0]);
void dump_memalloc(void)
	int ma;
	char *mt;
	for (ma = 0; ma < Nmemalloc; ma++) {
		switch (Memalloc[ma].memtype) {
		case MEM_DATA:
			mt = "data";
			break;
		case MEM_SHMEM:
			mt = "shmem";
			break;
		case MEM_MMAP:
			mt = "mmap";
			break;
		default:
			mt = "unknown";
			break;
		       Memalloc[ma].name, Memalloc[ma].nblks);
void parse_delay(char *arg)
	char *delayargs[NMEMALLOC];
	int ndelay;
	struct smap *s;
			     "Illegal delay arg (%s). Must be operation:time\n",
			     arg);
	for (s = delaymap; s->string != NULL; s++)
		fprintf(stderr, "Warning: extra delay arguments ignored.\n");
int usage(FILE * stream)
	fprintf(stream,
		"usage%s:  %s [-aekv] [-m message_interval] [-n nprocs] [-r release_interval] [-w write_log] [-V validation_ftype] [-U upanic_cond] [infile]\n",
		TagName, Prog);
void help(FILE * stream)
	fprintf(stream,
		"\t-a                   abort - kill all doio processes on data compare\n");
	fprintf(stream,
		"\t                     errors.  Normally only the erroring process exits\n");
	fprintf(stream,
		"\t                     Available data patterns are:\n");
	fprintf(stream,
		"\t                         select:time (1 second=1000000)\n");
	fprintf(stream,
		"\t                         sginap:time (1 second=CLK_TCK=100)\n");
	fprintf(stream,
		"\t-e                   Re-exec children before entering the main\n");
	fprintf(stream,
		"\t                     loop.  This is useful for spreading\n");
	fprintf(stream,
		"\t                     procs around on multi-pe systems.\n");
	fprintf(stream,
		"\t-k                   Lock file regions during writes using fcntl()\n");
	fprintf(stream,
		"\t-v                   Verify writes - this is done by doing a buffered\n");
	fprintf(stream,
		"\t                     read() of the data if file io was done, or\n");
	fprintf(stream,
		"\t                     an ssread()of the data if sds io was done\n");
	fprintf(stream,
		"\t-M                   Data buffer allocation method\n");
	fprintf(stream,
		"\t			        s - shared (shared file must exist\n"),
	    fprintf(stream,
		    "\t			            and have needed length)\n");
	fprintf(stream,
		"\t			        f - fixed address (not used)\n");
	fprintf(stream,
		"\t			        a - specify address (not used)\n");
	fprintf(stream,
		"\t			        U - Unlink file when done\n");
	fprintf(stream,
		"\t			        The default flag is private\n");
	fprintf(stream,
		"\t-m message_interval  Generate a message every 'message_interval'\n");
	fprintf(stream,
		"\t                     requests.  An interval of 0 suppresses\n");
	fprintf(stream,
		"\t                     messages.  The default is 0.\n");
	fprintf(stream,
		"\t-r release_interval  Release all memory and close\n");
	fprintf(stream,
		"\t                     files every release_interval operations.\n");
	fprintf(stream,
		"\t                     By default procs never release memory\n");
	fprintf(stream,
		"\t                     or close fds unless they have to.\n");
	fprintf(stream,
		"\t-V validation_ftype  The type of file descriptor to use for doing data\n");
	fprintf(stream,
		"\t                     validation.  validation_ftype may be an octal,\n");
	fprintf(stream,
		"\t                     hex, or decimal number representing the open()\n");
	fprintf(stream,
		"\t                     flags, or may be one of the following strings:\n");
	fprintf(stream,
		"\t                     'buffered' - validate using bufferd read\n");
	fprintf(stream,
		"\t                     'sync'     - validate using O_SYNC read\n");
	fprintf(stream,
		"\t                     'direct    - validate using O_DIRECT read'\n");
	fprintf(stream,
		"\t                     'ldraw'    - validate using O_LDRAW read\n");
	fprintf(stream,
		"\t                     'parallel' - validate using O_PARALLEL read\n");
	fprintf(stream,
		"\t                     'raw'      - validate using O_RAW read\n");
	fprintf(stream,
		"\t                     is used if the write was done with O_PARALLEL\n");
	fprintf(stream,
		"\t                     or 'buffered' for all other writes.\n");
	fprintf(stream,
		"\t-w write_log         File to log file writes to.  The doio_check\n");
	fprintf(stream,
		"\t                     program can reconstruct datafiles using the\n");
	fprintf(stream,
		"\t                     write_log, and detect if a file is corrupt\n");
	fprintf(stream,
		"\t                     after all procs have exited.\n");
	fprintf(stream,
		"\t-U upanic_cond       Comma separated list of conditions that will\n");
	fprintf(stream,
		"\t                     cause a call to upanic(PA_PANIC).\n");
	fprintf(stream,
		"\t                     'corruption' -> upanic on bad data comparisons\n");
	fprintf(stream,
		"\t                     'iosw'     ---> upanic on unexpected async iosw\n");
	fprintf(stream,
		"\t                     'rval'     ---> upanic on unexpected syscall rvals\n");
	fprintf(stream,
		"\t                     'all'      ---> all of the above\n");
	fprintf(stream,
		"\tinfile               Input stream - default is stdin - must be a list\n");
	fprintf(stream,
		"\t                     of io_req structures (see doio.h).  Currently\n");
	fprintf(stream,
		"\t                     only the iogen program generates the proper\n");