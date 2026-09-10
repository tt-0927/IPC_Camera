/***
 * @FilePath     : Singleton.h
 * @Author       : zjc
 * @Date         : 2022-07-01 16:43:26
 * @LastEditors  : huangjunda
 * @LastEditTime : 2025-03-27 11:04:11
 * @Description  : 单例模式模板
 */

#ifndef SINGLETON_H_
#define SINGLETON_H_

#include <mutex>
#include <memory>

template <class T>
class CSingleton
{
public:
    // 获取实例
    static T *instance()
    {
        std::call_once(m_instantiated, []()
                       { m_instance = std::shared_ptr<T>(new T); });
        return m_instance.get();
    }

    // 禁用拷贝和移动操作
    CSingleton(const CSingleton &) = delete;
    CSingleton &operator=(const CSingleton &) = delete;
    CSingleton(CSingleton &&) = delete;
    CSingleton &operator=(CSingleton &&) = delete;

protected:
    // 构造函数仅供子类访问
    CSingleton() = default;
    virtual ~CSingleton() = default;

private:
    static std::shared_ptr<T> m_instance;
    static std::once_flag m_instantiated;
};

// 静态成员初始化
template <class T>
std::shared_ptr<T> CSingleton<T>::m_instance = nullptr;
template <class T>
std::once_flag CSingleton<T>::m_instantiated;

/*
class Test : public CSingleton<Test>
{
public:
    // 单例模板作为友元类
    friend class CSingleton<Test>;

    void print() const
    {
        std::cout << "Value: " << id << " (Instance address: " << this << ")\n";
    }

    void setId(int id)
    {
        this->id = id;
    }

    int getId() const
    {
        return this->id;
    }
private:
    // 构造函数声明为私有
    Test() = default;
    ~Test() = default;

    int id = 0;
};

int main()
{
    // 验证单例特性
    auto& instance1 = Test::instance();
    instance1.setId(42);

    auto& instance2 = Test::instance();
    instance2.print();  // 输出相同实例地址和值

    // 验证地址一致性
    std::cout << "Address check: " << (&instance1 == &instance2) << "\n";  // 输出1

    return 0;
}
*/

#endif
