//========================================================
//
//    Copyright (c) 2008,�������߹�����
//    All rights reserved.
//
//    �ļ����ƣ�Stack.h
//    ժ    Ҫ��ջģ����
//
//    ��ǰ�汾��1.00
//    ��    �ߣ�����
//    ������ڣ�2008-3-19
//    �޸�˵����
//
//========================================================
#ifndef _FILE_STACK_H_
#define _FILE_STACK_H_

#include <deque>
#include <stdexcept>
#include <memory>

//****************************************************
//���֣���Ĭ�ϱ�׼����ʵ�ֵ�ջ
//������ջģ����
//���ڣ�2008-4-3
//****************************************************
namespace sbase
{
	template <typename T, template <typename ELEM, typename = std::allocator<ELEM> > class CONT = std::deque>
	class Stack
	{
		/*-------------------------------------------*/
		/*               ��Ա������               */
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
		/*               ��Ա������               */
		/*-------------------------------------------*/
	public:
	protected:
	private:
		/*----------------------------------------------------------
		  ���ܣ�Ԫ������
		  ���ߣ�����
		  ���ڣ�2008-4-3
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