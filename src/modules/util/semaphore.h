// SPDX-FileCopyrightText: 2018 - 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef LINGLONG_BOX_SRC_UTIL_SEMAPHORE_H_
#define LINGLONG_BOX_SRC_UTIL_SEMAPHORE_H_

#include <memory>

namespace linglong {

class Semaphore
{
public:
    explicit Semaphore(int key);
    ~Semaphore();

    int init();

    //! passeren -1 to value
    //! if value < 0, block
    //! \return
    int passeren();

    //! passeren +1 to value
    //! if value <= 0, release process in queue
    //! \return
    int vrijgeven();

private:
    struct SemaphorePrivate;
    std::unique_ptr<SemaphorePrivate> dd_ptr;
};

} // namespace linglong

#endif /* LINGLONG_BOX_SRC_UTIL_SEMAPHORE_H_ */
