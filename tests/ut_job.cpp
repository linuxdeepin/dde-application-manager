// SPDX-FileCopyrightText: 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: LGPL-3.0-or-later

#include "dbus/jobservice.h"
#include <gtest/gtest.h>
#include <QFuture>
#include <QPromise>
#include <thread>
#include <chrono>
#include <QSemaphore>

using namespace std::chrono_literals;

class TestJob : public testing::Test
{
public:
    void SetUp() override
    {
        QPromise<QVariantList> promise;
        m_jobService = new JobService{promise.future()};

        m_thread = QThread::create(
            [](QPromise<QVariantList> promise) {
                std::this_thread::sleep_for(2ms);
                promise.start();
                std::this_thread::sleep_for(1ms);
                if (promise.isCanceled()) {
                    return;
                }

                std::this_thread::sleep_for(1ms);
                promise.suspendIfRequested();
                std::this_thread::sleep_for(1ms);

                std::size_t x{0};
                for (std::size_t i = 0; i < 20000; ++i) {
                    x += 1;
                }

                promise.addResult(QVariantList{QVariant::fromValue<std::size_t>(x)});
                promise.finish();
            },
            std::move(promise));
    }

    void TearDown() override
    {
        m_thread->quit();
        m_thread->deleteLater();
        m_thread = nullptr;

        m_jobService->deleteLater();
        m_jobService = nullptr;
    }

    QThread *m_thread{nullptr};
    JobService *m_jobService{nullptr};
};

TEST_F(TestJob, cancelJob)
{
    m_thread->start();

    std::this_thread::sleep_for(1ms);
    EXPECT_EQ(m_jobService->status().toStdString(), std::string{"pending"});
    std::this_thread::sleep_for(2ms);
    EXPECT_EQ(m_jobService->status().toStdString(), std::string{"running"});

    m_jobService->Cancel();

    std::this_thread::sleep_for(1ms);

    EXPECT_EQ(m_jobService->status().toStdString(), std::string{"canceled"});
}

TEST_F(TestJob, suspendAndResumeJob)
{
    m_thread->start();

    std::this_thread::sleep_for(3ms);

    EXPECT_EQ(m_jobService->status().toStdString(), std::string{"running"});

    m_jobService->Suspend();

    std::this_thread::sleep_for(1ms);

    EXPECT_EQ(m_jobService->status().toStdString(), std::string{"suspending"});
    std::this_thread::sleep_for(1ms);
    EXPECT_EQ(m_jobService->status().toStdString(), std::string{"suspended"});
    m_jobService->Resume();

    std::this_thread::sleep_for(1ms);
    EXPECT_EQ(m_jobService->status().toStdString(), std::string{"running"});

    EXPECT_TRUE(m_thread->wait());
    EXPECT_EQ(m_jobService->status().toStdString(), std::string{"finished"});

    auto ret = m_jobService->m_job.result();
    EXPECT_FALSE(ret.isEmpty());

    EXPECT_EQ(ret.first().value<std::size_t>(), std::size_t{20000});
}
