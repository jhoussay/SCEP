#pragma once
//
#include <SCEP_CORE/SCEP_CORE.h>
//
#include <QString>
//
typedef long HRESULT;
//
/**
 *	@ingroup				SCEP
 *	@brief					Returns the last Win32 error, in string format.
 *	@return					Returns an empty string if there is no error.
 */
SCEP_CORE_DLL QString		GetLastErrorAsString();
/**
 *	@ingroup				SCEP
 *	@brief					Returns the HRESULT error, in string format.
 *	@return					Returns an empty string if there is no error.
 */
SCEP_CORE_DLL QString		GetErrorAsString(HRESULT hr);
//
template <typename T>
class Box
{
public:
	Box(T* t = nullptr)
		:	p_t(t)
	{}

	~Box()
	{
		clear();
	}

	Box(const Box& other) = delete;
	Box& operator =(const Box& other) = delete;

	Box(Box&& other)
	{
		clear();
		std::swap(p_t, other.p_t);
	}
	Box& operator =(Box&& other)
	{
		clear();
		std::swap(p_t, other.p_t);
		return *this;
	}

	Box& operator =(T* t)
	{
		clear();
		p_t = t;
		return *this;
	}

	void clear()
	{
		if (p_t)
		{
			p_t->Release();
			p_t = nullptr;
		}
	}

	T* get()
	{
		return p_t;
	}
	const T* get() const
	{
		return p_t;
	}

	T* operator ->()
	{
		return p_t;
	}
	const T* operator ->() const
	{
		return p_t;
	}

	T** operator &()
	{
		return &p_t;
	}
	const T** operator &() const
	{
		return &p_t;
	}

	operator bool() const
	{
		return p_t != nullptr;
	}

private:
	T* p_t = nullptr;
};
