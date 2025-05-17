//========================================================
//
//    Copyright (c) 2008,欢乐连线工作室
//    All rights reserved.
//
//    文件名称：Stack.h
//    摘    要：栈模版类
//
//    当前版本：1.00
//    作    者：李锋军
//    完成日期：2008-3-19
//    修改说明：
//
//========================================================
#ifndef _FILE_STACK_H_
#define _FILE_STACK_H_

#include <deque>
#include <stdexcept>
#include <memory>

//****************************************************
//名字：用默认标准容器实现的栈
//描述：栈模版类
//日期：2008-4-3
//****************************************************
namespace sbase
{
	template <typename T, template <typename ELEM, typename = std::allocator<ELEM> > class CONT = std::deque>
	class Stack
	{
		/*-------------------------------------------*/
		/*               成员方法区               */
		/*-------------------------------------------*/
	public:
		void push(T const&);
		void pop();
		T top() const;
		bool empty() const { return elems.empty(); }
		void clear() { elems.clear(); }
		template<typename T2, template<typename ELEM2, typename = std::allocator<ELEM2> >class CONT2 >
		Stack<T, CONT>& operator= (Stack<T2, CONT2> const&);
	protected:
	private:

		/*-------------------------------------------*/
		/*               成员变量区               */
		/*-------------------------------------------*/
	public:
	protected:
	private:
		/*----------------------------------------------------------
		  功能：元素容器
		  作者：李锋军
		  日期：2008-4-3
		----------------------------------------------------------*/
		CONT<T> elems;
	};

	template <typename T, template <typename, typename> class CONT>
	void Stack<T, CONT>::push(T const& elem)
	{
		elems.push_back(elem);
	}

	template<typename T, template <typename, typename> class CONT>
	void Stack<T, CONT>::pop()
	{
		if (elems.empty()) {
			throw std::out_of_range("Stack<>::pop(): empty stack");
		}
		elems.pop_back();
	}

	template <typename T, template <typename, typename> class CONT>
	T Stack<T, CONT>::top() const
	{
		if (elems.empty()) {
			throw std::out_of_range("Stack<>::top(): empty stack");
		}
		return elems.back();
	}

	template <typename T, template <typename, typename> class CONT>
	template <typename T2, template <typename, typename> class CONT2>
	Stack<T, CONT>&
		Stack<T, CONT>::operator= (Stack<T2, CONT2> const& op2)
	{
		if ((void*)this == (void*)&op2) {
			return *this;
		}

		Stack<T2, CONT2> tmp(op2);

		elems.clear();
		while (!tmp.empty())
		{
			elems.push_front(tmp.top());
			tmp.pop();
		}

		return *this;
	}
}

#endif