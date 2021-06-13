#pragma once

template<typename RetType, typename ...ArgTypes>
class DelegateBase
{
	template<typename, typename ...> 
	friend class Delegate;

	public:

	private:

		virtual RetType Call(ArgTypes...) = 0;
};

template<typename ClassType, typename MethodType, typename RetType, typename ...ArgTypes>
class DelegateImpl : public DelegateBase<RetType, ArgTypes...>
{
	public:

		DelegateImpl(ClassType* Object, MethodType Method) : Object(Object), Method(Method)
		{
		}

	private:

		ClassType *Object;
		MethodType Method;

		virtual RetType Call(ArgTypes... Args) override
		{
			return (Object->*Method)(Args...);
		}
};

template<typename RetType, typename ...ArgTypes>
class Delegate
{
	public:

		template<typename ClassType, typename MethodType>
		Delegate(ClassType* Object, MethodType Method)
		{
			new (DelegateStorage) DelegateImpl<ClassType, MethodType, RetType, ArgTypes...>(Object, Method);
		}

		RetType operator()(ArgTypes... Args)
		{
			return ((DelegateBase<RetType, ArgTypes...>*)DelegateStorage)->Call(Args...);
		}		

	private:

		BYTE DelegateStorage[2 * sizeof(void*)];
};