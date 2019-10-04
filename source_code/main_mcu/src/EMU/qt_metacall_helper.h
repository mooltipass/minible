#ifndef _QT_METACALL_HELPER
#define _QT_METACALL_HELPER

#include <QEvent>
#include <QCoreApplication>
#include <functional>
// https://github.com/KubaO/stackoverflown/tree/master/questions/metacall-21646467
namespace detail {
template <typename F>
struct FEvent : QEvent {
    using Fun = typename std::decay<F>::type;
    const QObject *const obj;
    Fun fun;
    template <typename Fun>
    FEvent(const QObject *_obj, Fun &&_fun) : QEvent(QEvent::None), obj(_obj), fun(std::forward<Fun>(_fun)) {}
    ~FEvent() { fun(); }
}; }

template <typename F>
static void postToObject(F &&fun, QObject *obj) {
    Q_ASSERT(obj);
    QCoreApplication::postEvent(obj, new detail::FEvent<F>(obj, std::forward<F>(fun)));
}

#endif
