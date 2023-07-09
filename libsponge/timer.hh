#ifndef SPONGE_LIBSPONGE_TIMER_HH
#define SPONGE_LIBSPONGE_TIMER_HH
class Timer
{
public:
    Timer():_duration(0),_started(false)
    { }
    size_t getDuration()
    {
        return _duration;
    }
    void addDuration(size_t ticks)
    {
        _duration=_duration+ticks;
    }
    bool isStart()
    {
        return _started;
    }
    void setStart()
    {
        _started=true;
    }
    void stop()
    {
        _started=false;
    }
    void reset()
    {
        _duration=0;
    }
private:
    size_t _duration;
    bool _started;
};

#endif