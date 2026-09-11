#ifndef KHICAS_INTEGRATION_GUARD_H
#define KHICAS_INTEGRATION_GUARD_H

// The calculator has one CAS evaluation thread. Keep only shallow gen handles
// on the existing call stack; do not allocate a cache of previous integrals.
namespace giac {
  class integration_guard {
    const gen expression_, variable_;
    const context * context_;
    const int mode_;
    const unsigned depth_;
    integration_guard * previous_;
    static integration_guard * active_;

  public:
    bool allowed;
    integration_guard(const gen & e, const gen & x, int mode, const context * ctx)
        : expression_(e), variable_(x), context_(ctx), mode_(mode),
          depth_(active_ ? active_->depth_ + 1 : 1), previous_(active_), allowed(true) {
      // Bound nested heuristic searches, not the number of terms in a sum.
      if (depth_ > 24)
        allowed = false;
      for (integration_guard * p = previous_; allowed && p; p = p->previous_)
        if (p->context_ == ctx && p->mode_ == mode && p->variable_ == x && p->expression_ == e)
          allowed = false;
      active_ = this;
    }
    ~integration_guard() {
      active_ = previous_;
    }

  private:
    integration_guard(const integration_guard &);
    integration_guard & operator=(const integration_guard &);
  };
} // namespace giac
#endif
