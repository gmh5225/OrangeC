#pragma once
#include "JobServerAwareThread.h"
#include "IJobServer.h"
#include <atomic>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <utility>
#include <memory>
#include "semaphores.h"
// The main issue with using semaphores alone for Linux is that named semaphores
// on linux can't be used without knowing the state of every application taking
// the named semaphores, a solution to this is to use a jobserver, in order for
// GNU Make compatability we'll use
// https://www.gnu.org/software/make/manual/html_node/Job-Slots.html#Job-Slots
// as a reference here to show how we can "give and take" slots, not only this,
// but using this as a compatability guide should be a good thing

namespace OMAKE
{
class POSIXJobServer;
class WINDOWSJobServer;
// Master job server that allows us to properly have polymorphic job servers, I believe this is the correct method to create a
// job-server for our purposes,
class JobServer : public IJobServer
{
  protected:
    // The maximum number of jobs, can be any value we like so long as it's above or equal to one
    int max_jobs;
    // The current job count, starts at one and goes up, protected so that we know how many we need to release, and all calls assume
    // that you have the minimum number of jobs (1) thus, current_jobs starts at 1
    std::atomic<int> current_jobs;
  public:
    // This is *REALLY* ineffecient, but is the only *easy* way to do this on POSIX without taking an unfathomably long amount of
    // extra time AFAICT
    virtual void ReleaseAllJobs()
    {
        while (current_jobs > 1)
        {
            ReleaseJob();
        }
    }
    virtual std::string PassThroughCommandString() = 0;
    // The JobServerAwareThread is aware of the calling jobserver, thus having a wrapper in the job server allows us to create
    // threads which will update our worker numbers easily
    template <class Function, class... Args>
    JobServerAwareThread CreateNewThread(Function&& f, Args&&... args)
    {
        // Threads start upon creation, thus we need to take a job before the thread is created to reserve the slot before work
        // starts *NO MATTER WHAT*, also prevents us from having to have 2 things in the IJobServer... (Could be changed to work so
        // that the thread itself does this(?))
        return JobServerAwareThread(job_server_instance, std::forward<Function>(f), std::forward<Args>(args)...);
    }
    // Creates a job server with a maximum number of jobs
    static std::shared_ptr<JobServer> GetJobServer(int max_jobs);
    // Opens a job server with a specified authorization code, this is *AFAR* parsing the --jobserver-auth string
    static std::shared_ptr<JobServer> GetJobServer(const std::string& auth_string);
    // temporary GetJobServer for compatibility reasons with old code, instead of moving things into JobServer here, we're moving
    // them out
    static std::shared_ptr<JobServer> GetJobServer(const std::string& auth_string, int max_jobs);
};
// Use composition to our advantage: if we have a JobServer and we know it's either one of these, we can use the same code
// without having to worry if it's POSIX or Windows except at the calling barrier
class POSIXJobServer : public JobServer
{
    friend class JobServer;
    int readfd = -1, writefd = -1;
    POSIXJobServer(int max_jobs);
    POSIXJobServer(int read, int write);
    std::string PassThroughCommandString();
    int get_read_fd() { return readfd; }
    int get_write_fd() { return writefd; }
    int TakeNewJob();
    int ReleaseJob();
};
class WINDOWSJobServer : public JobServer
{
    friend class JobServer;
#ifdef _WIN32
#    ifdef UNICODE
    // Use a consistent strategy so that windows doesn't yell at us, I know we don't work in wide strings except in obrc often
    // so this is here for posterity *AND* ease of use
    using string_type = std::wstring;
#    else
    using string_type = std::string;
#    endif
#else
    using string_type = std::string;
#endif
  private:
    // Name of the server, use std::string or std::wstring, we do this for compatibility reasons
    string_type server_name;
    // Since we already have a semaphore class that's sufficient, might as well use it so that we aren't carrying around a raw
    // HANDLE, this way we can also elide calls in this place and theoretically an optimization pass could bring the entire object
    // into the class in the future should anyone choose to research inserting an underlying object into another object
    Semaphore semaphore;

  public:
    // Since we use semaphores we want a name because these are interprocess named semaphores
    WINDOWSJobServer(const string_type& server_name, int max_jobs);
    // Interprocess semaphores require names
    WINDOWSJobServer(const string_type& server_name);
    std::string PassThroughCommandString();
    int TakeNewJob();
    int ReleaseJob();
    void ReleaseAllJobs();
};
}  // namespace OMAKE