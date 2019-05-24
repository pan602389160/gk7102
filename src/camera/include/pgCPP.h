/****************************************************************
  copyright   : Copyright (C) 2015, chenbichao, All rights reserved.
                www.peergine.com, www.pptun.com
                
  filename    : pgCPP.h
  discription : 
  modify      : create, chenbichao, 2015/03/28

*****************************************************************/
#ifndef _PG_CPP_H
#define _PG_CPP_H

#ifdef _PG_DLL_EXPORT
#define PG_DLL_API __declspec(dllexport)
#else
#define PG_DLL_API
#endif


///
// Relative interface declare.
class IPGString;
class IPGCPPNode;


///
// The CPP string class.
class PG_DLL_API CPGCPPString
{
public:
	char At(unsigned int uPos);

	const char* CStr() const;

	unsigned int Length();

	unsigned int Assign(const char* lpszStr, unsigned int uSize);
	unsigned int Assign(const char* lpszStr);

	unsigned int Append(const char* lpszStr);

	unsigned int Replace(unsigned int uPos0, unsigned int uSize0, const char* lpszStr);

	int Find(const char* lpszStr, unsigned int uPos = 0);
	int Find(char c, unsigned int uPos = 0);
	int FindFirstOf(const char* lpszStr, unsigned int uPos = 0);
	int FindFirstOf(char c, unsigned int uPos = 0);
	int FindLastOf(const char* lpszStr, unsigned int uPos = 0xffffffff);
	int FindLastOf(char c, unsigned int uPos = 0xffffffff);

	int Compare(unsigned int uPos0, unsigned int uSize0, const char* lpszStr) const;

	CPGCPPString SubStr(unsigned int uPos = 0, unsigned int uSize = 0xffffffff);

	CPGCPPString& operator=(const char* p) {
		Assign(p);
		return (*this);
	}
	CPGCPPString& operator=(const CPGCPPString& s) {
		Assign(s.CStr());
		return (*this);
	}

	CPGCPPString& operator+=(const char* p);
	CPGCPPString& operator+=(const CPGCPPString& s) {
		(*this) += s.CStr();
		return (*this);
	}

	unsigned int operator==(const char* p) const;
	unsigned int operator==(const CPGCPPString& s) const {
		return ((*this) == s.CStr());
	}

	unsigned int operator!=(const char* p) const {
		return !((*this) == p);
	}
	unsigned int operator!=(const CPGCPPString& s) const  {
		return ((*this) != s.CStr());
	}

	operator const char*() const {
		return CStr();
	}
	
	IPGString* GetIPGString() const {
		return m_pStr;
	}

	void Attach(IPGString* lpStr);	
	IPGString* Detach();

	CPGCPPString();
	CPGCPPString(const char* lpszStr);
	CPGCPPString(const CPGCPPString& s);
	virtual ~CPGCPPString();

private:
	IPGString* m_pStr;
};

PG_DLL_API
CPGCPPString operator+(const char* p, CPGCPPString& s);

PG_DLL_API
CPGCPPString operator+(const CPGCPPString& s, const char* p);

PG_DLL_API
CPGCPPString operator+(const CPGCPPString& s, const CPGCPPString& s1);



///
// The node extend message proc.
class PG_DLL_API IPGCPPNodeExtMsgProc {
public:
	virtual unsigned int OnPostMessage(IPGCPPNode* lpNode,
		unsigned int uNodeID, unsigned int uMsg, unsigned int uHandle) = 0;
};


///
// The node callback intetface class.
class PG_DLL_API IPGCPPNodeProc {
public:
	virtual unsigned int OnReply(const char* lpszObj, unsigned int uErrCode,
		const char* lpszData, const char* lpszParam) = 0;

	virtual int OnExtRequest(const char* lpszObj, unsigned int uMethod,
		const char* lpszData, unsigned int uHandle, const char* lpszPeer) = 0;
};


///
// The node interface class.
class PG_DLL_API IPGCPPNode
{
public:
	// The node property
	CPGCPPString Control;
	CPGCPPString Class;
	CPGCPPString Server;
	CPGCPPString Local;
	CPGCPPString Relay;
	CPGCPPString Node;

public:
	// The node callback interface.
	IPGCPPNodeProc* NodeProc;

public:
	// The OML Parser methods.
	virtual CPGCPPString omlEncode(const char* lpszEle) = 0;
	virtual CPGCPPString omlDecode(const char* lpszEle) = 0;
	virtual CPGCPPString omlSetName(const char* lpszEle, const char* lpszPath, const char* lpszName) = 0;
	virtual CPGCPPString omlSetClass(const char* lpszEle, const char* lpszPath, const char* lpszClass) = 0;
	virtual CPGCPPString omlSetContent(const char* lpszEle, const char* lpszPath, const char* lpszContent) = 0;
	virtual CPGCPPString omlNewEle(const char* lpszName, const char* lpszClass, const char* lpszContent) = 0;
	virtual CPGCPPString omlGetEle(const char* lpszEle, const char* lpszPath, int uSize, int uPos) = 0;
	virtual CPGCPPString omlDeleteEle(const char* lpszEle, const char* lpszPath, int uSize, int uPos) = 0;
	virtual CPGCPPString omlGetName(const char* lpszEle, const char* lpszPath) = 0;
	virtual CPGCPPString omlGetClass(const char* lpszEle, const char* lpszPath) = 0;
	virtual CPGCPPString omlGetContent(const char* lpszEle, const char* lpszPath) = 0;
	virtual CPGCPPString omlInsertEle(const char* lpszEle, const char* lpszPath, int uPos,
		const char* lpszName, const char* lpszClass, const char* lpszContent) = 0;

	// Object handle methods.
	virtual int ObjectAdd(const char* lpszName, const char* lpszClass, const char* lpszGroup, int uFlag) = 0;
	virtual void ObjectDelete(const char* lpszObj) = 0;
	virtual CPGCPPString ObjectEnum(const char* lpszObject, const char* lpszClass) = 0;
	virtual CPGCPPString ObjectGetClass(const char* lpszObj) = 0;
	virtual int ObjectSetGroup(const char* lpszObj, const char* lpszGroup) = 0;
	virtual CPGCPPString ObjectGetGroup(const char* lpszObj) = 0;
	virtual int ObjectSync(const char* lpszObj, const char* lpszPeer, int uAction) = 0;
	virtual int ObjectRequest(const char* lpszObj, int uMethod, const char* lpszIn, const char* lpszParam) = 0;
	virtual int ObjectExtReply(const char* lpszObj, int uErrCode, const char* lpszOut, int uHandle) = 0;

	// Utilize methods.
	virtual CPGCPPString utilCmd(const char* lpszCmd, const char* lpszParam) = 0;
	virtual CPGCPPString utilGetWndRect() = 0;

	// Node contel methods.
	virtual int Start(int uOption) = 0;
	virtual void Stop() = 0;
	virtual int PostMessage(const char* lpszMsg) = 0;
	virtual int PumpMessage(unsigned int uLoop) = 0;
	virtual void Quit() = 0;

	// Node extend message proc.
	virtual void ExtMsgProcAttach(unsigned int uNodeID, IPGCPPNodeExtMsgProc* lpProc) = 0;
	virtual IPGCPPNodeExtMsgProc* ExtMsgProcDetach() = 0;
	virtual unsigned int ExtMsgGetNodeID() = 0;
	virtual unsigned int OnExtMsgPost(unsigned int uMsg, unsigned int uHandle) = 0;

	// Delete this node object.
	virtual void Delete() = 0;
};

///
// New a node object.
PG_DLL_API
IPGCPPNode* pgNewCPPNode();

///
// Log output callback.
typedef void (*TfnPGCPPLogOut)(unsigned int uLevel, const char* lpszOut);
PG_DLL_API
void pgCPPSetLogCallback(TfnPGCPPLogOut pfnCallback);

///
// Node lib initialize
PG_DLL_API
unsigned int pgCPPInitialize();

// Node lib clean
PG_DLL_API
void pgCPPClean();


#endif //_PG_CPP_H
