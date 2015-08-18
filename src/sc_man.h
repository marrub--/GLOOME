#ifndef __SC_MAN_H__
#define __SC_MAN_H__

class FScanner
{
public:
	struct SavedPos
	{
		const char *SavedScriptPtr;
		int SavedScriptLine;
	};

	// Methods ------------------------------------------------------
	FScanner();
	FScanner(const FScanner &other);
	FScanner(int lumpnum);
	~FScanner();

	FScanner &operator=(const FScanner &other);

	void Open(const char *lumpname);
	void OpenFile(const char *filename);
	void OpenMem(const char *name, const char *buffer, int size);
	void OpenString(const char *name, FString buffer);
	void OpenLumpNum(int lump);
	void Close();

	void SetCMode(bool cmode);
	void SetEscape(bool esc);
	const SavedPos SavePos();
	void RestorePos(const SavedPos &pos);

	static FString TokenName(int token, const char *string=NULL);

	bool GetString();
	void MustGetString();
	void MustGetStringName(const char *name);
	bool CheckString(const char *name);

	bool GetToken();
	void MustGetAnyToken();
	void TokenMustBe(int token);
	void MustGetToken(int token);
	bool CheckToken(int token);
	bool CheckTokenId(ENamedName id);

	bool GetNumber();
	void MustGetNumber();
	bool CheckNumber();

	bool GetFloat();
	void MustGetFloat();
	bool CheckFloat();

	void UnGet();

	bool Compare(const char *text);
	int MatchString(const char * const *strings, size_t stride = sizeof(char*));
	int MustMatchString(const char * const *strings, size_t stride = sizeof(char*));
	int GetMessageLine();

	void ScriptError(const char *message, ...);
	void ScriptMessage(const char *message, ...);

	bool isText();

	// Members ------------------------------------------------------
	char *String;
	int StringLen;
	int TokenType;
	int Number;
	double Float;
	FName Name;
	int Line;
	bool End;
	bool Crossed;
	int LumpNum;
	FString ScriptName;

protected:
	void PrepareScript();
	void CheckOpen();
	bool ScanString(bool tokens);

	// Strings longer than this minus one will be dynamically allocated.
	static const int MAX_STRING_SIZE = 128;

	bool ScriptOpen;
	FString ScriptBuffer;
	const char *ScriptPtr;
	const char *ScriptEndPtr;
	char StringBuffer[MAX_STRING_SIZE];
	FString BigStringBuffer;
	bool AlreadyGot;
	int AlreadyGotLine;
	bool LastGotToken;
	const char *LastGotPtr;
	int LastGotLine;
	bool CMode;
	bool Escape;
};

enum
{
	TK_SequenceStart = 256,
#define xx(sym,str) sym,
#include "sc_man_tokens.h"
	TK_LastToken
};


//==========================================================================
//
//
//
//==========================================================================

enum
{
	MSG_WARNING,
	MSG_FATAL,
	MSG_ERROR,
	MSG_DEBUG,
	MSG_LOG,
	MSG_DEBUGLOG,
	MSG_MESSAGE
};

//==========================================================================
//
// a class that remembers a parser position
//
//==========================================================================

struct FScriptPosition
{
	static int ErrorCounter;
	FString FileName;
	int ScriptLine;

	FScriptPosition()
	{
		ScriptLine=0;
	}
	FScriptPosition(const FScriptPosition &other);
	FScriptPosition(FString fname, int line);
	FScriptPosition(FScanner &sc);
	FScriptPosition &operator=(const FScriptPosition &other);
	void Message(int severity, const char *message,...) const;
	static void ResetErrorCounter()
	{
		ErrorCounter = 0;
	}
};


#endif //__SC_MAN_H__
