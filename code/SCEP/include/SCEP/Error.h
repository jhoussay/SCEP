#pragma once
//
#include <memory>
#include <iostream>
//
#include <QString>
//
using Error = QString;
//
using ErrorPtr = std::shared_ptr<Error>;
//
inline ErrorPtr createError(const QString& error)
{
	return std::make_shared<Error>(error);
}
//
inline ErrorPtr success()
{
	return {};
}
//
inline void displayError(ErrorPtr pError)
{
	if (pError)
		std::cerr << pError->toStdString() << std::endl;
}
//
inline bool isError(ErrorPtr pError)
{
	return pError ? (! pError->isEmpty()) : false;
}
//
#define CALL(call) { ErrorPtr pMyTmpError = (call); if (isError(pMyTmpError)) { return pMyTmpError; } }
//
#define CHECK(condition, error) { if (! (condition)) { return createError(error); } }
//