#ifndef _ACLUI_H
#define _ACLUI_H

#ifdef __ORANGEC__ 
#pragma once
#endif

/* Windows ACLUI.DLL definitions */

#include <objbase.h>
#include <commctrl.h>
#include <accctrl.h>

#define ACLUIAPI  DECLSPEC_IMPORT WINAPI

#ifdef __cplusplus
extern "C" {
#endif

#define SI_EDIT_PERMS  0x00000000L
#define SI_EDIT_OWNER  0x00000001L
#define SI_EDIT_AUDITS  0x00000002L
#define SI_CONTAINER  0x00000004L
#define SI_READONLY  0x00000008L
#define SI_ADVANCED  0x00000010L
#define SI_RESET  0x00000020L
#define SI_OWNER_READONLY  0x00000040L
#define SI_EDIT_PROPERTIES  0x00000080L
#define SI_OWNER_RECURSE  0x00000100L
#define SI_NO_ACL_PROTECT  0x00000200L
#define SI_NO_TREE_APPLY  0x00000400L
#define SI_PAGE_TITLE  0x00000800L
#define SI_SERVER_IS_DC  0x00001000L
#define SI_RESET_DACL_TREE  0x00004000L
#define SI_RESET_SACL_TREE  0x00008000L
#define SI_OBJECT_GUID  0x00010000L
#define SI_EDIT_EFFECTIVE  0x00020000L
#define SI_RESET_DACL  0x00040000L
#define SI_RESET_SACL  0x00080000L
#define SI_RESET_OWNER  0x00100000L
#define SI_NO_ADDITIONAL_PERMISSION  0x00200000L
#if (NTDDI_VERSION >= NTDDI_VISTA)
#define SI_VIEW_ONLY  0x00400000L
#define SI_PERMS_ELEVATION_REQUIRED  0x01000000L
#define SI_AUDITS_ELEVATION_REQUIRED  0x02000000L
#define SI_OWNER_ELEVATION_REQUIRED  0x04000000L
#if (NTDDI_VERSION >= NTDDI_WIN8)
#define SI_SCOPE_ELEVATION_REQUIRED 0x08000000L
#endif /* NTDDI_VERSION >= NTDDI_WIN8 */
#endif /* NTDDI_VERSION >= NTDDI_VISTA */
#define SI_MAY_WRITE  0x10000000L
#if (NTDDI_VERSION >= NTDDI_WIN8)
#define SI_ENABLE_EDIT_ATTRIBUTE_CONDITION  0x20000000L
#define SI_ENABLE_CENTRAL_POLICY  0x40000000L
#define SI_DISABLE_DENY_ACE  0x80000000L
#endif /* NTDDI_VERSION >= NTDDI_WIN8 */

#define SI_EDIT_ALL  (SI_EDIT_PERMS|SI_EDIT_OWNER|SI_EDIT_AUDITS)

#define SI_ACCESS_SPECIFIC  0x00010000L
#define SI_ACCESS_GENERAL  0x00020000L
#define SI_ACCESS_CONTAINER  0x00040000L
#define SI_ACCESS_PROPERTY  0x00080000L

#define PSPCB_SI_INITDIALOG  (WM_USER+1)

#define CFSTR_ACLUI_SID_INFO_LIST  TEXT("CFSTR_ACLUI_SID_INFO_LIST")

typedef struct _SI_OBJECT_INFO {
    DWORD dwFlags;
    HINSTANCE hInstance;
    LPWSTR pszServerName;
    LPWSTR pszObjectName;
    LPWSTR pszPageTitle;
    GUID guidObjectType;
} SI_OBJECT_INFO, *PSI_OBJECT_INFO;

typedef struct _SI_ACCESS {
    const GUID *pguid;
    ACCESS_MASK mask;
    LPCWSTR pszName;
    DWORD dwFlags;
} SI_ACCESS, *PSI_ACCESS;

typedef struct _SI_INHERIT_TYPE {
    const GUID *pguid;
    ULONG dwFlags;
    LPCWSTR pszName;
} SI_INHERIT_TYPE, *PSI_INHERIT_TYPE;

typedef enum _SI_PAGE_TYPE {
    SI_PAGE_PERM=0,
    SI_PAGE_ADVPERM,
    SI_PAGE_AUDIT,
    SI_PAGE_OWNER,
    SI_PAGE_EFFECTIVE,
#if (NTDDI_VERSION >= NTDDI_VISTA)
    SI_PAGE_TAKEOWNERSHIP,
#endif /* NTDDI_VERSION >= NTDDI_VISTA */
#if (NTDDI_VERSION >= NTDDI_WIN8)
    SI_PAGE_SHARE,
#endif /* NTDDI_VERSION >= NTDDI_WIN8 */
} SI_PAGE_TYPE;

#undef INTERFACE
#define INTERFACE ISecurityInformation
DECLARE_INTERFACE_(ISecurityInformation, IUnknown)
{
    STDMETHOD(QueryInterface)(THIS_ REFIID,LPVOID*) PURE;
    STDMETHOD_(ULONG,AddRef)(THIS) PURE;
    STDMETHOD_(ULONG,Release)(THIS) PURE;
    STDMETHOD(GetObjectInformation)(THIS_ PSI_OBJECT_INFO) PURE;
    STDMETHOD(GetSecurity)(THIS_ SECURITY_INFORMATION,PSECURITY_DESCRIPTOR*,BOOL) PURE;
    STDMETHOD(SetSecurity)(THIS_ SECURITY_INFORMATION,PSECURITY_DESCRIPTOR) PURE;
    STDMETHOD(GetAccessRights)(THIS_ const GUID*,DWORD,PSI_ACCESS*,ULONG*,ULONG*) PURE;
    STDMETHOD(MapGeneric) (THIS_ const GUID*,UCHAR*,ACCESS_MASK*) PURE;
    STDMETHOD(GetInheritTypes)(THIS_ PSI_INHERIT_TYPE*,ULONG*) PURE;
    STDMETHOD(PropertySheetPageCallback)(THIS_ HWND,UINT,SI_PAGE_TYPE) PURE;
};
typedef ISecurityInformation *LPSECURITYINFO;

#undef INTERFACE
#define INTERFACE ISecurityInformation2
DECLARE_INTERFACE_(ISecurityInformation2, IUnknown)
{
    STDMETHOD(QueryInterface)(THIS_ REFIID,LPVOID*) PURE;
    STDMETHOD_(ULONG,AddRef)(THIS) PURE;
    STDMETHOD_(ULONG,Release)(THIS) PURE;
    STDMETHOD_(BOOL,IsDaclCanonical)(THIS_ PACL) PURE;
    STDMETHOD(LookupSids)(THIS_ ULONG,PSID*,LPDATAOBJECT*) PURE;
};
typedef ISecurityInformation2 *LPSECURITYINFO2;

typedef struct _SID_INFO {
    PSID pSid;
    PWSTR pwzCommonName;
    PWSTR pwzClass;
    PWSTR pwzUPN;
} SID_INFO, *PSID_INFO;

typedef struct _SID_INFO_LIST {
    ULONG cItems;
    SID_INFO aSidInfo[ANYSIZE_ARRAY];
} SID_INFO_LIST, *PSID_INFO_LIST;

#undef INTERFACE
#define INTERFACE IEffectivePermission
DECLARE_INTERFACE_(IEffectivePermission, IUnknown)
{
    STDMETHOD(QueryInterface)(THIS_ REFIID, LPVOID*) PURE;
    STDMETHOD_(ULONG,AddRef)(THIS) PURE;
    STDMETHOD_(ULONG,Release)(THIS) PURE;
    STDMETHOD(GetEffectivePermission)(THIS_ const GUID*,PSID,LPCWSTR,PSECURITY_DESCRIPTOR,POBJECT_TYPE_LIST*,ULONG*,PACCESS_MASK*,ULONG*) PURE;
};
typedef IEffectivePermission *LPEFFECTIVEPERMISSION;

#undef INTERFACE
#define INTERFACE ISecurityObjectTypeInfo
DECLARE_INTERFACE_(ISecurityObjectTypeInfo, IUnknown)
{
    STDMETHOD(QueryInterface)(THIS_ REFIID, LPVOID*) PURE;
    STDMETHOD_(ULONG,AddRef)(THIS) PURE;
    STDMETHOD_(ULONG,Release)(THIS) PURE;
    STDMETHOD(GetInheritSource)(SECURITY_INFORMATION,PACL,PINHERITED_FROM*) PURE;
};
typedef ISecurityObjectTypeInfo *LPSecurityObjectTypeInfo;

#if (NTDDI_VERSION >= NTDDI_VISTA)

#undef INTERFACE
#define INTERFACE  ISecurityInformation3
DECLARE_INTERFACE_IID_(ISecurityInformation3, IUnknown, "E2CDC9CC-31BD-4f8f-8C8B-B641AF516A1A")
{
    STDMETHOD(QueryInterface)(THIS_ REFIID, void **) PURE;
    STDMETHOD_(ULONG, AddRef)(THIS) PURE;
    STDMETHOD_(ULONG, Release)(THIS) PURE;
    STDMETHOD(GetFullResourceName)(THIS_ LPWSTR *) PURE;
    STDMETHOD(OpenElevatedEditor)(THIS_ HWND, SI_PAGE_TYPE) PURE;
};
typedef ISecurityInformation3 *LPSECURITYINFO3;
#endif /* NTDDI_VERSION >= NTDDI_VISTA */

#if (NTDDI_VERSION >= NTDDI_WIN8)

typedef struct _SECURITY_OBJECT {
    PWSTR pwszName;
    PVOID pData;
    DWORD cbData;
    PVOID pData2;
    DWORD cbData2;
    DWORD Id;
    BOOLEAN fWellKnown;
} SECURITY_OBJECT, *PSECURITY_OBJECT;

#define SECURITY_OBJECT_ID_OBJECT_SD  1
#define SECURITY_OBJECT_ID_SHARE  2
#define SECURITY_OBJECT_ID_CENTRAL_POLICY  3
#define SECURITY_OBJECT_ID_CENTRAL_ACCESS_RULE  4

typedef struct _EFFPERM_RESULT_LIST {
    BOOLEAN fEvaluated;
    ULONG cObjectTypeListLength;
    OBJECT_TYPE_LIST *pObjectTypeList;
    ACCESS_MASK *pGrantedAccessList;
} EFFPERM_RESULT_LIST, *PEFFPERM_RESULT_LIST;

#undef INTERFACE
#define INTERFACE  ISecurityInformation4
DECLARE_INTERFACE_IID_(ISecurityInformation4, IUnknown, "EA961070-CD14-4621-ACE4-F63C03E583E4")
{
    STDMETHOD(QueryInterface)(THIS_ REFIID, void **) PURE;
    STDMETHOD_(ULONG, AddRef)(THIS) PURE;
    STDMETHOD_(ULONG, Release)(THIS) PURE;
    STDMETHOD(GetSecondarySecurity)(THIS_ PSECURITY_OBJECT *, PULONG) PURE;
};

typedef ISecurityInformation4 *LPSECURITYINFO4;

#undef INTERFACE
#define INTERFACE  IEffectivePermission
DECLARE_INTERFACE_IID_(IEffectivePermission2, IUnknown, "941FABCA-DD47-4FCA-90BB-B0E10255F20D")
{
    STDMETHOD(QueryInterface)(THIS_ REFIID, void **) PURE;
    STDMETHOD_(ULONG, AddRef)(THIS) PURE;
    STDMETHOD_(ULONG, Release)(THIS) PURE;
    STDMETHOD(ComputeEffectivePermissionWithSecondarySecurity)(THIS_ PSID, PSID, PCWSTR, PSECURITY_OBJECT, DWORD, PTOKEN_GROUPS, PAUTHZ_SID_OPERATION, PTOKEN_GROUPS, PAUTHZ_SID_OPERATION, PAUTHZ_SECURITY_ATTRIBUTES_INFORMATION, PAUTHZ_SECURITY_ATTRIBUTE_OPERATION, PAUTHZ_SECURITY_ATTRIBUTES_INFORMATION, PAUTHZ_SECURITY_ATTRIBUTE_OPERATION, PEFFPERM_RESULT_LIST);
};
typedef IEffectivePermission2 *LPEFFECTIVEPERMISSION2;
#endif /* NTDDI_VERSION >= NTDDI_WIN8 */

EXTERN_GUID(IID_ISecurityInformation,0x965fc360,0x16ff,0x11d0,0x91,0xcb,0x0,0xaa,0x0,0xbb,0xb7,0x23);
EXTERN_GUID(IID_ISecurityInformation2,0xc3ccfdb4,0x6f88,0x11d2,0xa3,0xce,0x0,0xc0,0x4f,0xb1,0x78,0x2a);
EXTERN_GUID(IID_IEffectivePermission,0x3853dc76,0x9f35,0x407c,0x88,0xa1,0xd1,0x93,0x44,0x36,0x5f,0xbc);
EXTERN_GUID(IID_ISecurityObjectTypeInfo,0xfc3066eb,0x79ef,0x444b,0x91,0x11,0xd1,0x8a,0x75,0xeb,0xf2,0xfa);
#if (NTDDI_VERSION >= NTDDI_VISTA)
EXTERN_GUID(IID_ISecurityInformation3,0xe2cdc9cc,0x31bd,0x4f8f,0x8c,0x8b,0xb6,0x41,0xaf,0x51,0x6a,0x1a);
#endif /* NTDDI_VERSION >= NTDDI_VISTA */
#if (NTDDI_VERSION >= NTDDI_WIN8)
EXTERN_GUID(IID_ISecurityInformation4,0xea961070,0xcd14,0x4621,0xac,0xe4,0xf6,0x3c,0x3,0xe5,0x83,0xe4);
EXTERN_GUID(IID_IEffectivePermission2,0x941fabca,0xdd47,0x4fca,0x90,0xbb,0xb0,0xe1,0x2,0x55,0xf2,0xd);
#endif /* NTDDI_VERSION >= NTDDI_WIN8 */

HPROPSHEETPAGE ACLUIAPI CreateSecurityPage(LPSECURITYINFO);
BOOL ACLUIAPI EditSecurity(HWND,LPSECURITYINFO);

#if (NTDDI_VERSION >= NTDDI_VISTA)
HRESULT ACLUIAPI EditSecurityAdvanced(HWND, LPSECURITYINFO, SI_PAGE_TYPE);
#endif /* NTDDI_VERSION >= NTDDI_VISTA */

#ifdef __cplusplus
}
#endif

#endif  /* _ACLUI_H */