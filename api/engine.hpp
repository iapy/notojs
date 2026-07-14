#pragma once
#include <quickjs/quickjs.h>
#include <boost/config.hpp>
#include <memory>

namespace notojs {

class Global;

class Task : public std::enable_shared_from_this<Task>
{
protected:
    virtual ~Task() {}
    
    enum Step
    {
        Again = 0,
        Finish = 1
    }; 

    virtual Step step() = 0;

    enum Then
    {
        Reject = 0,
        Resolve = 1
    };
  
    virtual Then then(JSContext *ctx, JSValue &res) = 0;

    static thread_local char buffer[16384];
public:
    void end(JSContext *);
    JSValue run(JSContext *);

    BOOST_FORCEINLINE operator bool () const
    {
        return !thread;
    }

private:
    void run();
    JSValue funcs[2];

    void   *thread{nullptr};
    Global *global{nullptr};
};

} // namespace notojs
