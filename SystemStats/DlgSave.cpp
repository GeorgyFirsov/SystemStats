// Project files
#include "stdafx.h"
#include "DlgSave.h"
#include "Exception.h"
#include "resource.h"
#include "i18n.h"
#include "Utils.h"

// STL headers
#include <string>
#include <regex>
#include <numeric>


IMPLEMENT_DYNAMIC(CDlgSave, CDialog)


/* Message handlers registration */

BEGIN_MESSAGE_MAP(CDlgSave, CDialog)
    ON_BN_CLICKED(IDC_SAVEDLG_RDSPCF, OnRdChoosePressed)
    ON_BN_CLICKED(IDC_SAVEDLG_RDALL, OnRdAllPressed)
END_MESSAGE_MAP()


BOOL CDlgSave::OnInitDialog()
{
    utils::CWaitCursor wc;

    CDialog::OnInitDialog();

    //
    // Set initial state of elements
    // 

    m_hEditFile.SetWindowText(system_stats::i18n::LoadUIString(IDS_DUMPFILE));
    m_hEditRecords.SetWindowText(system_stats::i18n::LoadUIString(IDS_DEFAULTRANGE));

    m_hRdAll.SetCheck(BST_CHECKED);

    return TRUE;
}

void CDlgSave::DDV_GoodRange(CDataExchange* pDX)
{
    if (!pDX->m_bSaveAndValidate) {
        return;
    }

    if (!IsRangeValid()) {
        AfxThrowUserException();
    }
}

bool CDlgSave::IsRangeValid()
{
    utils::CWaitCursor wc;

    namespace exc = system_stats::exception;

    try
    {
        WCHAR buffer[MAX_PATH + 1] = { 0 };
        m_hEditRecords.GetLine(0, buffer, _countof(buffer) - 1);

        //
        // We need to check input string. First chech for
        // emptiness, two consequential commas and leading comma
        // (assume trailing comma to be OK).
        // 

        CString sText(buffer);
        sText.Trim();

        if (sText.IsEmpty() || sText[0] == L',' || sText.Find(L",,") != -1) {
            ERROR_THROW_CODE(ERROR_INVALID_INDEX, system_stats::i18n::LoadUIString(IDS_ERROR_INVALIDRANGE));
        }

        //
        // String passed simple checks, now we need to tokenize
        // it (token is a sequence between two commas) and ensure
        // that all tokens represent a number or a range.
        // 

        std::wregex SubrangeRegex(LR"(^([1-9]\d*|[1-9]\d*-[1-9]\d*)$)");

        int iTokenBegin = 0;
        for (CString sToken = ""; !sToken.IsEmpty() || iTokenBegin != -1; sToken = sText.Tokenize(L",", iTokenBegin))
        {
            if (iTokenBegin == 0) {
                continue;
            }

            sToken.Replace(L" ", L"");

            if (sText.IsEmpty() || !regex_match(std::wstring(sToken), SubrangeRegex)) {
                ERROR_THROW_CODE(ERROR_INVALID_INDEX, system_stats::i18n::LoadUIString(IDS_ERROR_INVALIDRANGE));
            }

            //
            // If token is a range - check this range validity:
            // the beginning must be less or equal to the end
            // 

            if (sToken.Find(L'-') != -1)
            {
                int iNumberBegin = 0;
                CString sBegin = sToken.Tokenize(L"-", iNumberBegin);
                CString sEnd = sToken.Tokenize(L"-", iNumberBegin);

                size_t ullBegin = std::stoull(std::wstring(sBegin));
                size_t ullEnd = std::stoull(std::wstring(sEnd));

                if (ullBegin > ullEnd) {
                    ERROR_THROW_CODE(ERROR_INVALID_INDEX, system_stats::i18n::LoadUIString(IDS_ERROR_INVALIDRANGE));
                }
            }
        }

        return true;
    }
    catch (const exc::CWin32Error& error)
    {
        exc::DisplayErrorMessage(error, m_hWnd);
        return false;
    }
}

void CDlgSave::DoDataExchange(CDataExchange* pDX)
{
    CDialog::DoDataExchange(pDX);

    if (m_hRdChoose.GetCheck() == BST_CHECKED) {
        DDV_GoodRange(pDX);
    }

    DDX_Control(pDX, IDC_SAVEDLG_FILENAME, m_hEditFile);
    DDX_Control(pDX, IDC_SAVEDLG_RDALL, m_hRdAll);
    DDX_Control(pDX, IDC_SAVEDLG_RDSPCF, m_hRdChoose);
    DDX_Control(pDX, IDC_SAVEDLG_RECORDS, m_hEditRecords);
}


/* CDlgSave message handlers */

afx_msg void CDlgSave::OnRdChoosePressed()
{
    if (m_hRdChoose.GetCheck() == BST_CHECKED) {
        m_hEditRecords.EnableWindow(TRUE);
    }
}

afx_msg void CDlgSave::OnRdAllPressed()
{
    if (m_hRdAll.GetCheck() == BST_CHECKED) {
        m_hEditRecords.EnableWindow(FALSE);
    }
}

afx_msg void CDlgSave::OnOK()
{
    if (!IsRangeValid()) {
        return;
    }

    WCHAR file[MAX_PATH + 1];
    int iWritten = m_hEditFile.GetLine(0, file, _countof(file) - 1);

    m_wsFileName.assign(file, static_cast<size_t>(iWritten));

    if (m_hRdAll.GetCheck() == BST_CHECKED) 
    {
        m_Indexes.clear();
    }
    else
    {
        WCHAR buffer[MAX_PATH + 1] = { 0 };
        m_hEditRecords.GetLine(0, buffer, _countof(buffer) - 1);

        //
        // Assume here, that the range is validated
        // 

        CString sRange(buffer);
        sRange.Replace(L" ", L"");

        int iTokenBegin = 0;
        for (CString sToken = ""; !sToken.IsEmpty() || iTokenBegin != -1; sToken = sRange.Tokenize(L",", iTokenBegin))
        {
            if (iTokenBegin == 0) {
                continue;
            }

            //
            // Now include all listed indexes into resulting set
            // 

            if (sToken.Find(L'-') == -1)
            {
                m_Indexes.insert(std::stoull(std::wstring(sToken)) - 1);
            }
            else
            {
                int iNumberBegin = 0;
                CString sBegin = sToken.Tokenize(L"-", iNumberBegin);
                CString sEnd   = sToken.Tokenize(L"-", iNumberBegin);

                auto InsertIter = m_Indexes.end();
                for (size_t num = std::stoull(std::wstring(sBegin)); num <= std::stoull(std::wstring(sEnd)); num++) {
                    m_Indexes.insert(InsertIter, num - 1);
                }
            }
        }
    }

    CDialog::OnOK();
}