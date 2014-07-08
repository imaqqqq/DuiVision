#include "StdAfx.h"
#include <mmsystem.h> 
#include "ControlBase.h"

#pragma comment(lib,"Winmm.lib")

// 控件ID自动生成变量，控件ID从1000开始生成
static int g_nControlId = 1000;

CControlBase::CControlBase(HWND hWnd, CDuiObject* pDuiObject)
{
	m_pParentDuiObject = pDuiObject;
	m_hWnd = hWnd;
	m_uID = g_nControlId++;
	if(g_nControlId > 20000)	// 超过某个值之后重置
	{
		g_nControlId = 10000;
	}
	m_rc = CRect(0,0,0,0);
	m_strPos = "";
	m_bIsVisible = TRUE;
	m_bIsDisable = FALSE;
	m_bRresponse = TRUE;
	m_bTabStop = FALSE;
	m_bUpdate = FALSE;
	m_nDCWidth = 0;
	m_nDCHeight = 0;
	m_nAniDCWidth = 0;
	m_nAniDCHeight = 0;
	m_pControl = NULL;

	m_bMouseDown = false;
	m_bDblClk = false;
	m_bIsSelect = false;
	m_bIsRun = false;
	m_bRunTime = false;
	m_bImageUseECM = false;

	m_nShortcutKey = 0;
	m_nShortcutFlag = 0;

	m_nWidth = 0;
	m_nHeight = 0;

	m_strTooltip = _T("");
	m_strAction = _T("");
	m_bTaskMsg = FALSE;
}

CControlBase::CControlBase(HWND hWnd, CDuiObject* pDuiObject, UINT uControlID, CRect rc, BOOL bIsVisible, BOOL bIsDisable,
						   BOOL bRresponse)
{
	m_pParentDuiObject = pDuiObject;
	m_hWnd = hWnd;
	m_uID = uControlID;
	m_rc = rc;
	m_bIsVisible = bIsVisible;
	m_bIsDisable = bIsDisable;
	m_bRresponse = bRresponse;
	m_bTabStop = FALSE;
	m_bUpdate = FALSE;
	m_nDCWidth = 0;
	m_nDCHeight = 0;
	m_nAniDCWidth = 0;
	m_nAniDCHeight = 0;
	m_pControl = NULL;

	m_bMouseDown = false;
	m_bDblClk = false;
	m_bIsSelect = false;
	m_bIsRun = false;
	m_bRunTime = false;
	m_bImageUseECM = false;

	m_nWidth = 0;
	m_nHeight = 0;

	m_strTooltip = _T("");
	m_strAction = _T("");
	m_bTaskMsg = FALSE;
}

CControlBase::~CControlBase(void)
{
	for (size_t i = 0; i < m_vecControl.size(); i++)
	{
		CControlBase * pControlBase = m_vecControl.at(i);
		if (pControlBase)
		{
			delete pControlBase;
		}		
	}

	for (size_t i = 0; i < m_vecArea.size(); i++)
	{
		CControlBase * pControlBase = m_vecArea.at(i);
		if (pControlBase)
		{
			delete pControlBase;
		}		
	}
	
	// 释放内存DC
	if(m_bitmap.m_hObject)
	{
		if(m_memDC.m_hDC)
		{
			m_memDC.SelectObject(m_pOldBitmap);
			m_memDC.DeleteDC();
		}
		m_bitmap.DeleteObject();
	}

	// 释放动画DC
	if(m_aniBitmap.m_hObject)
	{
		if(m_animateDC.m_hDC)
		{
			m_animateDC.SelectObject(m_pOldAniBitmap);
			m_animateDC.DeleteDC();
		}
		m_aniBitmap.DeleteObject();
	}
}

// 判断当前是否在主线程
void CControlBase::TestMainThread()
{
	// 调用所在对话框的测试函数
	CDlgBase* pDlg = GetParentDialog();
	if(pDlg)
	{
		pDlg->TestMainThread();
	}
}

void CControlBase::Draw(CDC &dc, CRect rcUpdate)
{
	if(m_bIsVisible && !(m_rc & rcUpdate).IsRectEmpty())
	{
		DrawControl(dc, rcUpdate);

		// 画子区域
		for (size_t i = 0; i < m_vecArea.size(); i++)
		{
			CControlBase * pControlBase = m_vecArea.at(i);
			if (pControlBase)
			{
				pControlBase->Draw(dc, rcUpdate);
			}
		}

		// 画子控件
		DrawSubControls(dc, rcUpdate);
	}
}

// 画子控件
BOOL CControlBase::DrawSubControls(CDC &dc, CRect rcUpdate)
{
	for (size_t i = 0; i < m_vecControl.size(); i++)
	{
		CControlBase * pControlBase = m_vecControl.at(i);
		if (pControlBase && pControlBase->GetVisible())
		{
			pControlBase->Draw(dc, rcUpdate);		
		}
	}

	return TRUE;
}

void CControlBase::SetUpdate(BOOL bUpdate, COLORREF clr/* = 0*/)
{
	m_bUpdate = bUpdate;

	for (size_t i = 0; i < m_vecControl.size(); i++)
	{
		CControlBase * pControlBase = m_vecControl.at(i);
		if (pControlBase)
		{
			pControlBase->SetUpdate(bUpdate, clr);			
		}
	}
}

// 更新内存DC
void CControlBase::UpdateMemDC(CDC &dc, int nWidth, int nHeight)
{
	m_bUpdate = true;
	
	if(m_nDCWidth != nWidth || m_nDCHeight != nHeight)
	{		
		m_nDCWidth = nWidth;
		m_nDCHeight = nHeight;
		if(m_bitmap.m_hObject)
		{
			if(m_memDC.m_hDC)
			{
				m_memDC.SelectObject(m_pOldBitmap);
				m_memDC.DeleteDC();
			}
			m_bitmap.DeleteObject();			
		}

		m_memDC.CreateCompatibleDC(&dc);
		m_bitmap.CreateCompatibleBitmap(&dc, m_nDCWidth, m_nDCHeight);
		m_pOldBitmap = m_memDC.SelectObject(&m_bitmap);
	}
}

// 更新动画DC
void CControlBase::UpdateAnimateDC(CDC &dc, int nWidth, int nHeight)
{
	//m_bUpdate = true;
	
	//if(m_nAniDCWidth != nWidth || m_nAniDCHeight != nHeight)
	{		
		m_nAniDCWidth = nWidth;
		m_nAniDCHeight = nHeight;
		if(m_aniBitmap.m_hObject)
		{
			if(m_animateDC.m_hDC)
			{
				m_animateDC.SelectObject(m_pOldAniBitmap);
				m_animateDC.DeleteDC();
			}
			m_aniBitmap.DeleteObject();			
		}

		m_animateDC.CreateCompatibleDC(&dc);
		m_aniBitmap.CreateCompatibleBitmap(&dc, m_nAniDCWidth, m_nAniDCHeight);
		m_pOldAniBitmap = m_animateDC.SelectObject(&m_aniBitmap);
	}
}

BOOL CControlBase::PtInRect(CPoint point)
{
	if(!m_bIsVisible) return false;

	return m_rc.PtInRect(point);
}

// 设置焦点
BOOL CControlBase::SetWindowFocus()
{
	CDlgBase* pDlg = GetParentDialog();
	if(pDlg)
	{
		pDlg->SetFocus();
		return TRUE;
	}

	return FALSE;
}

// 设置控件焦点
BOOL CControlBase::OnFocus(BOOL bFocus)
{
	return SetControlFocus(bFocus);
}

// 判断当前控件是否焦点控件
BOOL CControlBase::IsFocusControl()
{
	CDlgBase* pDlg = GetParentDialog(FALSE);
	if(pDlg)
	{
		if(pDlg->GetFocusControl() == this)
		{
			return TRUE;
		}
	}

	return FALSE;
}

// 获取当前焦点控件
CControlBase* CControlBase::GetFocusControl(CControlBase* pFocusControl)
{
	for (int i = m_vecControl.size()-1; i >= 0; i--)
	{
		CControlBase* pControlBase = m_vecControl.at(i);
		if (pControlBase && pControlBase->GetVisible() && !pControlBase->GetDisable() && (pControlBase == pFocusControl) && pControlBase->IsTabStop())
		{
			return pControlBase;
		}else
		if (pControlBase && (pControlBase == m_pControl))
		{
			// 查找子控件
			pControlBase = pControlBase->GetFocusControl(pFocusControl);
			if(pControlBase != NULL)
			{
				return pControlBase;
			}
		}
	}

	return NULL;
}

// 获取上一个可以获取焦点的子控件
CControlBase* CControlBase::GetPrevFocusableControl(CControlBase* pFocusControl)
{
	BOOL bStartSearch = FALSE;
	// 先按照焦点控件查找一次
	for (int i = m_vecControl.size()-1; i >= 0; i--)
	{
		CControlBase* pControlBase = m_vecControl.at(i);
		if (pControlBase && pControlBase->GetVisible() && !pControlBase->GetDisable() && bStartSearch && pControlBase->IsTabStop())
		{
			return pControlBase;			
		}else
		if (pControlBase && (pControlBase == pFocusControl))
		{
			bStartSearch = TRUE;
		}
	}

	// 再遍历子控件查找
	if(m_pControl == NULL)
	{
		bStartSearch = TRUE;
	}
	for (int i = m_vecControl.size()-1; i >= 0; i--)
	{
		CControlBase* pControlBase = m_vecControl.at(i);
		if(m_pControl == NULL)
		{
			m_pControl = pControlBase;
		}
		if (pControlBase && pControlBase->GetVisible() && !pControlBase->GetDisable() && bStartSearch && pControlBase->IsTabStop())
		{
			return pControlBase;			
		}else
		if (pControlBase && (pControlBase == m_pControl))
		{
			// 查找子控件
			pControlBase = pControlBase->GetPrevFocusableControl(pFocusControl);
			if(pControlBase != NULL)
			{
				return pControlBase;
			}
		}
	}

	return NULL;
}

// 获取下一个可以获取焦点的子控件
CControlBase* CControlBase::GetNextFocusableControl(CControlBase* pFocusControl)
{
	BOOL bStartSearch = FALSE;
	// 先按照焦点控件查找一次
	for (int i = 0; i < m_vecControl.size(); i++)
	{
		CControlBase* pControlBase = m_vecControl.at(i);
		if (pControlBase && pControlBase->GetVisible() && !pControlBase->GetDisable() && bStartSearch && pControlBase->IsTabStop())
		{
			return pControlBase;			
		}else
		if (pControlBase && (pControlBase == pFocusControl))
		{
			bStartSearch = TRUE;
		}
	}

	// 再遍历子控件查找
	if(m_pControl == NULL)
	{
		bStartSearch = TRUE;
	}
	for (int i = 0; i < m_vecControl.size(); i++)
	{
		CControlBase* pControlBase = m_vecControl.at(i);
		if(m_pControl == NULL)
		{
			m_pControl = pControlBase;
		}
		if (pControlBase && pControlBase->GetVisible() && !pControlBase->GetDisable() && bStartSearch && pControlBase->IsTabStop())
		{
			return pControlBase;			
		}else
		if (pControlBase && (pControlBase == m_pControl))
		{
			// 查找子控件
			pControlBase = pControlBase->GetNextFocusableControl(pFocusControl);
			if(pControlBase != NULL)
			{
				return pControlBase;
			}
		}
	}

	return NULL;
}

// 鼠标移动事件处理
BOOL CControlBase::OnMouseMove(UINT nFlags, CPoint point)
{
	if(!m_bIsVisible || !m_bRresponse) return false;
	
	OnMousePointChange(point);

	BOOL bRresponse = false;
	if(m_pControl)
	{
		if((m_pControl->PtInRect(point) && m_pControl->OnCheckMouseResponse(nFlags, point)) || m_bMouseDown)
		{
			if(m_pControl->OnMouseMove(nFlags, point))
			{
				return true;
			}
			return false;
		}
	}

	CControlBase * pOldControl = m_pControl;
	m_pControl =  NULL;

	if(!m_strTooltip.IsEmpty() && PtInRect(point) && OnCheckMouseResponse(nFlags, point))
	{
		// 如果当前控件有Tooltip,则添加一个Tooltip
		CDlgBase* pDlg = GetParentDialog();
		if(pDlg && (pDlg->GetTooltipCtrlID() != GetID()))
		{
			pDlg->SetTooltip(this, m_strTooltip, m_rc);
			pDlg->SetTooltipCtrlID(GetID());
		}
	}

	bRresponse = OnControlMouseMove(nFlags, point);

	if(!m_bMouseDown)
	{
		for (size_t i = 0; i < m_vecControl.size(); i++)
		{
			CControlBase * pControlBase = m_vecControl.at(i);
			if (pControlBase)
			{
				if(pControlBase->OnMouseMove(nFlags, point))
				{
					if(pControlBase->PtInRect(point))
					{
						m_pControl = pControlBase;
					}
					bRresponse = true;
				}
			}
		}


		if (pOldControl)
		{
			bRresponse = true;
		}

		if (m_pControl)
		{
			bRresponse = true;
		}
	}

	return bRresponse;
}

// 鼠标左键事件处理
BOOL CControlBase::OnLButtonDown(UINT nFlags, CPoint point)
{
	if(!m_bIsVisible || !m_bRresponse) return false;

	OnMousePointChange(point);

	m_bMouseDown = m_rc.PtInRect(point);

	// 查找鼠标是否在某个内部控件位置,如果是的话就更新当前子控件
	for (size_t i = 0; i < m_vecControl.size(); i++)
	{
		CControlBase * pControlBase = m_vecControl.at(i);
		if (pControlBase && pControlBase->PtInRect(point))
		{
			if(pControlBase->GetVisible() && !pControlBase->GetDisable() && pControlBase->GetRresponse())
			{
				m_pControl = pControlBase;
			}
		}
	}

	if(m_pControl != NULL)
	{
		if(m_pControl->OnLButtonDown(nFlags, point))
		{
			return true;
		}		
	}
	else
	{
		// 切换对话框中的当前焦点控件(暂不处理Popup窗口的切换)
		CDlgBase* pDlg = GetParentDialog(FALSE);
		if(pDlg)
		{
			pDlg->SetFocusControl(this);
		}

		return OnControlLButtonDown(nFlags, point);
	}

	return false;
}

// 鼠标左键事件处理
BOOL CControlBase::OnLButtonUp(UINT nFlags, CPoint point)
{
	if(!m_bIsVisible || !m_bRresponse) return false;

	OnMousePointChange(point);

	m_bMouseDown = false;
	if(m_pControl != NULL)
	{
		if(m_pControl->OnLButtonUp(nFlags, point))
		{
			return true;
		}		
	}
	else
	{
		return OnControlLButtonUp(nFlags, point);
	}

	return false;
}

// 滚动事件处理
BOOL CControlBase::OnScroll(BOOL bVertical, UINT nFlags, CPoint point)
{
	if(!m_bIsVisible || !m_bRresponse) return false;
	
	BOOL bRresponse = false;
	if(m_pControl)
	{
		// 判断当前活动控件
		if(m_pControl->PtInRect(point) && m_pControl->OnCheckMouseResponse(nFlags, point))
		{
			if(m_pControl->OnScroll(bVertical, nFlags, point))
			{
				return true;
			}
			return false;
		}
	}

	if(PtInRect(point))
	{
		// 在此控件范围内,先判断此控件是否能处理
		if(OnControlScroll(bVertical, nFlags, point))
		{
			return true;
		}else
		{
			// 此控件没有处理,则遍历子控件看是否能处理
			for (size_t i = 0; i < m_vecControl.size(); i++)
			{
				CControlBase * pControlBase = m_vecControl.at(i);
				if (pControlBase && pControlBase->PtInRect(point))
				{
					if(pControlBase->OnScroll(bVertical, nFlags, point))
					{
						return true;
					}
				}
			}
		}
	}

	return false;
}

// 键盘事件处理
BOOL CControlBase::OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags)
{
	if(!m_bIsVisible || !m_bRresponse) return false;
	
	BOOL bRresponse = false;
	// 判断当前活动控件
	if(m_pControl && m_pControl->OnKeyDown(nChar, nRepCnt, nFlags))
	{
		return true;
	}

	// 控件自身是否可以处理此事件
	if(OnControlKeyDown(nChar, nRepCnt, nFlags))
	{
		return true;
	}

	// 此控件没有处理,则遍历子控件看是否能处理
	for (size_t i = 0; i < m_vecControl.size(); i++)
	{
		CControlBase * pControlBase = m_vecControl.at(i);
		if (pControlBase && pControlBase->OnKeyDown(nChar, nRepCnt, nFlags))
		{
			return true;
		}
	}

	return false;
}

// 键盘事件处理
BOOL CControlBase::OnControlKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags)
{
	// 如果快捷键能够匹配控件的快捷键,则发送一个模拟鼠标点击的消息
	if((m_nShortcutKey != 0) && (nChar == m_nShortcutKey) && (nFlags == m_nShortcutFlag))
	{
		SendMessage(BUTTOM_DOWN, 0, 0);
		SendMessage(BUTTOM_UP, 0, 0);
		return true;
	}

	return false;
}

BOOL CControlBase::OnTimer()
{
	BOOL bRresponse = false;
	CRect rcnControlUpdate;

	bRresponse = OnControlTimer();

	for (size_t i = 0; i < m_vecControl.size(); i++)
	{
		CControlBase * pControlBase = m_vecControl.at(i);
		if (pControlBase)
		{
			rcnControlUpdate.SetRectEmpty();
			if(pControlBase->OnTimer())
			{
				bRresponse = true;
			}
		}
	}

	return bRresponse;
}

void CControlBase::SetRect(CRect rc)
{ 
	CRect rcAll = m_rc | rc;
	SetControlRect(rc);
	UpdateControl(rcAll, m_bIsVisible, true);
}

// 控件的位置信息需要刷新
void CControlBase::OnPositionChange()
{
	// 刷新控件自身的位置
	OnAttributePosChange(m_strPos, FALSE);

	// 刷新子控件的位置
	for (size_t i = 0; i < m_vecControl.size(); i++)
	{
		CControlBase * pControlBase = m_vecControl.at(i);
		if (pControlBase)
		{
			pControlBase->OnPositionChange();
		}
	}
}

// 计算位置信息的具体坐标值
int CControlBase::PositionItem2Value( const DUIDLG_POSITION_ITEM &pos ,int nMin, int nMax)
{
	int nRet=0;
	int nWid=nMax-nMin;

	switch(pos.pit)
	{
	case PIT_CENTER: 
		nRet=(int)pos.nPos * (pos.bMinus?-1:1) + nWid/2 + nMin;
		break;
	case PIT_NORMAL: 
		if(pos.bMinus)
			nRet=nMax-(int)pos.nPos;
		else
			nRet=nMin+(int)pos.nPos;
		break;
	case PIT_PERCENT: 
		nRet=nMin+(int)(pos.nPos*nWid/100);
		if(nRet>nMax) nRet=nMax;
		break;
	}
	
	return nRet;

}

// 解析位置信息
LPCSTR CControlBase::ParsePosition(const char * pszPos,DUIDLG_POSITION_ITEM &pos)
{
	if(!pszPos) return NULL;

	if(pszPos[0]=='|') pos.pit=PIT_CENTER,pszPos++;
	else if(pszPos[0]=='%') pos.pit=PIT_PERCENT,pszPos++;
	else pos.pit=PIT_NORMAL;
	
	if(pszPos[0]=='-') pos.bMinus=TRUE,pszPos++;
	else pos.bMinus=FALSE;

	pos.nPos=(float)atof(pszPos);

	const char *pNext=strchr(pszPos,',');
	if(pNext) pNext++;
	return pNext;
}

// 从XML设置位置信息属性
HRESULT CControlBase::OnAttributePosChange(const CStringA& strValue, BOOL bLoading)
{
    if (strValue.IsEmpty()) return E_FAIL;

	m_strPos = strValue;

	DUIDLG_POSITION dlgpos;

	dlgpos.nCount=0;
	LPCSTR pszValue=strValue;
	while(dlgpos.nCount<4 && pszValue)
	{
		pszValue=ParsePosition(pszValue,dlgpos.Item[dlgpos.nCount++]);
	}

    if (2 == dlgpos.nCount || 4 == dlgpos.nCount)
    {
		if(4 == dlgpos.nCount)
		{
			CRect rectParent = CRect(0,0,0,0);
			CDuiObject* pParent = GetParent();
			if(pParent)
			{
				rectParent = pParent->GetRect();
			}
			CRect rect;
			rect.left = PositionItem2Value(dlgpos.Left, rectParent.left, rectParent.right);
			rect.top = PositionItem2Value(dlgpos.Top, rectParent.top, rectParent.bottom);
			rect.right = PositionItem2Value(dlgpos.Right, rectParent.left, rectParent.right);
			rect.bottom = PositionItem2Value(dlgpos.Bottom, rectParent.top, rectParent.bottom);
			SetRect(rect);
		}
		else if(2 == dlgpos.nCount)
		{
			//m_uPositionType = (m_uPositionType & ~SizeX_Mask) | SizeX_FitContent;
			//m_uPositionType = (m_uPositionType & ~SizeY_Mask) | SizeY_FitContent;
			CRect rectParent = CRect(0,0,0,0);
			CDuiObject* pParent = GetParent();
			if(pParent)
			{
				rectParent = pParent->GetRect();
			}
			CRect rect;
			rect.left = PositionItem2Value(dlgpos.Left, rectParent.left, rectParent.right);
			rect.top = PositionItem2Value(dlgpos.Top, rectParent.top, rectParent.bottom);
			if(m_nWidth != 0)
			{
				rect.right = rect.left + m_nWidth;
			}else
			{
				rect.right = PositionItem2Value(dlgpos.Left, rectParent.left, rectParent.right);
			}
			if(m_nHeight != 0)
			{
				rect.bottom = rect.top + m_nHeight;
			}else
			{
				rect.bottom = PositionItem2Value(dlgpos.Top, rectParent.top, rectParent.bottom);
			}
			SetRect(rect);
		}
    }
    else
        dlgpos.nCount = 0;

    return bLoading?S_FALSE:S_OK;
}

// 从XML设置宽度信息属性
HRESULT CControlBase::OnAttributeWidth(const CStringA& strValue, BOOL bLoading)
{
    if (strValue.IsEmpty()) return E_FAIL;

	m_nWidth = atoi(strValue);
	m_rc.right = m_rc.left + m_nWidth;
	SetRect(m_rc);

	return bLoading?S_FALSE:S_OK;
}

// 从XML设置高度信息属性
HRESULT CControlBase::OnAttributeHeight(const CStringA& strValue, BOOL bLoading)
{
    if (strValue.IsEmpty()) return E_FAIL;

	m_nHeight = atoi(strValue);
	m_rc.bottom = m_rc.top + m_nHeight;
	SetRect(m_rc);

	return bLoading?S_FALSE:S_OK;
}

// 从XML设置快捷键信息属性
HRESULT CControlBase::OnAttributeShortcut(const CStringA& strValue, BOOL bLoading)
{
	CDuiObject::ParseKeyCode(strValue, m_nShortcutKey, m_nShortcutFlag);

	return bLoading?S_FALSE:S_OK;
}

void CControlBase::SetVisible(BOOL bIsVisible)
{
	SetControlVisible(bIsVisible);
	UpdateControl(true, true);
}

// 获取控件的可见性(遍历父控件,如果父控件不可见,则返回不可见)
BOOL CControlBase::IsControlVisible()
{
	if(!GetVisible())
	{
		return FALSE;
	}

	CDuiObject* pParentObj = GetParent();
	if(pParentObj == NULL)
	{
		return GetVisible();
	}

	if(pParentObj->IsClass("dlg") || pParentObj->IsClass("popup"))
	{
		return GetVisible();
	}

	return ((CControlBase*)pParentObj)->IsControlVisible();
}

void  CControlBase::SetDisable(BOOL bIsDisable)
{
	if(m_bIsDisable != bIsDisable)
	{
		SetControlDisable(bIsDisable);
		UpdateControl(true);
	}
}

void CControlBase::UpdateControl(BOOL bUpdate, BOOL bVisible)
{
	if(m_bIsVisible || bVisible)
	{
		m_pParentDuiObject->OnControlUpdate(m_rc, bUpdate, this);
	}
}

void CControlBase::UpdateControl(CRect rc, BOOL bVisible, BOOL bUpdate)
{
	if(m_bIsVisible || bVisible)
	{
		m_pParentDuiObject->OnControlUpdate(rc, bUpdate, this);
	}
}

void CControlBase::InvalidateRect(LPCRECT lpRect, BOOL bErase)
{
	CDuiObject* pParentObj = GetParent();
	while((pParentObj != NULL) && (!pParentObj->IsClass("dlg")))
	{
		pParentObj = ((CControlBase*)pParentObj)->GetParent();
	}

	if((pParentObj != NULL) && pParentObj->IsClass("dlg"))
	{
		((CDlgBase*)pParentObj)->InvalidateRect(lpRect, bErase);
	}
}

// 获取子控件对象
CControlBase *CControlBase::GetControl(UINT uControlID)
{
	for (size_t i = 0; i < m_vecControl.size(); i++)
	{
		CControlBase * pControlBase = m_vecControl.at(i);
		if (pControlBase)
		{
			if (pControlBase->GetControlID() == uControlID)
			{
				return pControlBase;
			}else
			{
				// 查找子控件
				CControlBase * pSubControl = pControlBase->GetControl(uControlID);
				if(pSubControl != NULL)
				{
					return pSubControl;
				}
			}
		}
	}

	for (size_t i = 0; i < m_vecArea.size(); i++)
	{
		CControlBase * pControlBase = m_vecArea.at(i);
		if (pControlBase)
		{
			if (pControlBase->GetControlID() == uControlID)
			{
				return pControlBase;
			}else
			{
				// 查找子控件
				CControlBase * pSubControl = pControlBase->GetControl(uControlID);
				if(pSubControl != NULL)
				{
					return pSubControl;
				}
			}
		}
	}

	return NULL;
}

// 获取子控件对象
CControlBase *CControlBase::GetControl(CString strControlName)
{
	for (size_t i = 0; i < m_vecControl.size(); i++)
	{
		CControlBase * pControlBase = m_vecControl.at(i);
		if (pControlBase)
		{
			if (pControlBase->GetName() == strControlName)
			{
				return pControlBase;
			}else
			{
				// 查找子控件
				CControlBase * pSubControl = pControlBase->GetControl(strControlName);
				if(pSubControl != NULL)
				{
					return pSubControl;
				}
			}
		}
	}

	for (size_t i = 0; i < m_vecArea.size(); i++)
	{
		CControlBase * pControlBase = m_vecArea.at(i);
		if (pControlBase)
		{
			if (pControlBase->GetName() == strControlName)
			{
				return pControlBase;
			}else
			{
				// 查找子控件
				CControlBase * pSubControl = pControlBase->GetControl(strControlName);
				if(pSubControl != NULL)
				{
					return pSubControl;
				}
			}
		}
	}

	return NULL;
}

// 添加子控件
void CControlBase::AddControl(CControlBase* pControl)
{
	if(pControl != NULL)
	{
		m_vecControl.push_back(pControl);
	}
}

// 清除指定的子控件
BOOL CControlBase::RemoveControl(CControlBase* pControl)
{
	vector<CControlBase*>::iterator it;
	for(it=m_vecControl.begin();it!=m_vecControl.end();++it)
	{
		CControlBase* pControlBase = *it;
		if (pControlBase == pControl)
		{
			m_vecControl.erase(it);
			delete pControlBase;
			return TRUE;
		}
	}

	return FALSE;
}

// 清除指定名字的子控件
BOOL CControlBase::RemoveControl(CString strControlName, UINT uControlID)
{
	vector<CControlBase*>::iterator it;
	for(it=m_vecControl.begin();it!=m_vecControl.end();++it)
	{
		CControlBase* pControlBase = *it;
		if (pControlBase && pControlBase->IsThisObject(uControlID, strControlName))
		{
			m_vecControl.erase(it);
			delete pControlBase;
			return TRUE;
		}
	}

	return FALSE;
}

// 清除指定类型的子控件
void CControlBase::RemoveControls(CStringA strClassName)
{
	for (int i = m_vecControl.size()-1; i >= 0; i--)
	{
		CControlBase* pControlBase = m_vecControl.at(i);
		if(pControlBase->IsClass(strClassName))
		{
			RemoveControl(pControlBase);
		}
	}
}

// 获取父对话框
CDlgBase* CControlBase::GetParentDialog(BOOL bEnablePopup)
{
	CDuiObject* pParentObj = GetParent();
	while((pParentObj != NULL) && (!pParentObj->IsClass("dlg")))
	{
		if(pParentObj->IsClass("popup"))
		{
			if(!bEnablePopup)	// 如果不允许Popup窗口的控件获取父对话框,则直接返回空
			{
				return NULL;
			}
			pParentObj = ((CDlgPopup*)pParentObj)->GetParent();
		}else
		{
			pParentObj = ((CControlBase*)pParentObj)->GetParent();
		}
	}

	if((pParentObj != NULL) && pParentObj->IsClass("dlg"))
	{
		return (CDlgBase*)pParentObj;
	}

	return NULL;
}

// 消息处理
LRESULT CControlBase::OnMessage(UINT uID, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	if(m_bTaskMsg)
	{
		// 如果设置了任务方式发消息的属性,则添加一个任务消息
		CString strControlName = GetName();
		CString strAction = GetAction();
		CDlgBase* pParentDlg = GetParentDialog();
		DuiSystem::Instance()->AddDuiActionTask(uID, uMsg, wParam, lParam, strControlName, strAction, pParentDlg);
		return 0;
	}

	if(m_strAction.Find(_T("dlg:")) == 0)	// 动作:打开一个对话框,有内存泄漏，改为通过DuiSystem创建和管理
	{
		if(uMsg == BUTTOM_UP)	// 鼠标放开事件才处理
		{
			CString strXmlFile = m_strAction;
			strXmlFile.Delete(0, 4);
			DuiSystem::ShowDuiDialog(strXmlFile, GetParentDialog());
		}
	}else
	if(m_strAction.Find(_T("popup:")) == 0)	// 动作:打开一个Popup对话框
	{
		if(uMsg == BUTTOM_UP)	// 鼠标放开事件才处理
		{
			/*UINT nIDTemplate = 0;
			CDlgBase* pParentDlg = GetParentDialog();
			if(pParentDlg != NULL)
			{
				nIDTemplate = pParentDlg->GetIDTemplate();
			}
			CDlgPopup* pPopup =  new CDlgPopup;
			pPopup->SetParent(this);
			CString strXmlFile = m_strAction;
			strXmlFile.Delete(0, 6);
			pPopup->SetXmlFile(_T("xml:") +strXmlFile );
			CRect rc = pControlBase->GetRect();
			rc.OffsetRect(-95, rc.Height());
			ClientToScreen(&rc);
			pPopup->Create(this, rc, WM_SKIN);
			pPopup->ShowWindow(SW_SHOW);*/
		}
	}else
	if(m_strAction.Find(_T("menu:")) == 0)	// 动作:打开一个菜单
	{
		CDuiMenu *pDuiMenu = new CDuiMenu( DuiSystem::GetDefaultFont(), 12);	// 可以考虑改为通过DuiSystem创建和管理
		pDuiMenu->SetParent(this);
		CPoint point;
		CRect rc = GetRect();
		point.SetPoint(rc.left + rc.Width() / 2, rc.bottom);
		CDlgBase* pParentDlg = GetParentDialog();
		if(pParentDlg != NULL)
		{
			pParentDlg->ClientToScreen(&point);
		}
		CString strXmlFile = m_strAction;
		strXmlFile.Delete(0, 5);
		pDuiMenu->LoadXmlFile(strXmlFile, pParentDlg, point, WM_DUI_MENU);
		pDuiMenu->ShowWindow(SW_SHOW);
	}else
	if(m_strAction.Find(_T("link:")) == 0)	// 动作:打开一个页面链接
	{
		if(uMsg == BUTTOM_UP)	// 鼠标放开事件才处理
		{
			CString strLink = m_strAction;
			strLink.Delete(0, 5);
			if(!strLink.IsEmpty())
			{
				ShellExecute(NULL, TEXT("open"), strLink, NULL,NULL,SW_NORMAL);
			}
		}
	}else
	if(m_strAction.Find(_T("run:")) == 0)	// 动作:执行一个进程
	{
		if(uMsg == BUTTOM_UP)	// 鼠标放开事件才处理
		{
			CString strProcess = m_strAction;
			strProcess.Delete(0, 4);
			strProcess.MakeLower();
			if(!strProcess.IsEmpty())
			{
				strProcess.MakeLower();
				BOOL bForceAdmin = FALSE;
				if(strProcess.Find(_T("admin@")) == 0)
				{
					bForceAdmin = TRUE;
					strProcess.Delete(0, 6);
				}
				BOOL bWaitProcess = FALSE;
				if(strProcess.Find(_T("&")) == (strProcess.GetLength()-1))
				{
					bWaitProcess = TRUE;
					strProcess.Delete(strProcess.GetLength()-1, 1);
				}
				if(strProcess.Find(_T(".exe")) == -1)
				{
					strProcess = DuiSystem::Instance()->GetString(CEncodingUtil::UnicodeToAnsi(strProcess));
				}
				if(strProcess.Find(_T("{platpath}")) == 0)
				{
					strProcess.Delete(0, 10);
					strProcess = DuiSystem::GetExePath() + strProcess;
				}
				CString strCmdLine = _T("");
				int nPos = strProcess.Find(_T("|"));
				if(nPos != -1)
				{
					strCmdLine = strProcess;
					strCmdLine.Delete(0, nPos+1);
					strProcess = strProcess.Left(nPos);
				}
				DuiSystem::PathCanonicalize(strProcess);	// 路径标准化
				DuiSystem::ExecuteProcess(strProcess, strCmdLine, bForceAdmin, bWaitProcess);
			}
		}
	}else
	if(m_strAction.Find(ACTION_CLOSE_WINDOW) == 0)	// 动作:关闭指定的窗口
	{
		if(uMsg == BUTTOM_UP)	// 鼠标放开事件才处理
		{
			CString strWndName = m_strAction;
			strWndName.Delete(0, 13);
			if(!strWndName.IsEmpty())
			{
				CDlgBase* pDlg = DuiSystem::Instance()->GetDuiDialog(strWndName);
				if(pDlg != NULL)
				{
					//pDlg->DoClose();
					pDlg->PostMessage(WM_QUIT, 0, 0);
				}
			}
		}
	}else
	{
		// 首先判断如果是几个默认按钮,则直接做相应的处理
		CDlgBase* pParentDlg = GetParentDialog();
		if(IsThisObject(BT_OK, NAME_BT_OK))
		{
			if((BUTTOM_UP == uMsg) && (pParentDlg != NULL)) { pParentDlg->DoOK(); }
		}else
		if(IsThisObject(BT_CANCEL, NAME_BT_CANCEL))
		{
			if((BUTTOM_UP == uMsg) && (pParentDlg != NULL)) { pParentDlg->DoCancel(); }
		}else
		if(IsThisObject(BT_YES, NAME_BT_YES))
		{
			if((BUTTOM_UP == uMsg) && (pParentDlg != NULL)) { pParentDlg->DoYes(); }
		}else
		if(IsThisObject(BT_NO, NAME_BT_NO))
		{
			if((BUTTOM_UP == uMsg) && (pParentDlg != NULL)) { pParentDlg->DoNo(); }
		}else
		{
			// 调用控件的DUI事件处理对象
			CallDuiHandler(uMsg, wParam, lParam);
		}
	}

	return 0;
}

// 调用DUI事件处理对象
LRESULT CControlBase::CallDuiHandler(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	CDuiHandler* pDuiHandler = GetDuiHandler();
	if(pDuiHandler != NULL)
	{
		return pDuiHandler->OnDuiMessage(GetID(), GetName(), uMsg, wParam, lParam);
	}

	CDuiObject* pParentObj = GetParent();
	while(pParentObj != NULL)
	{
		CDuiHandler* pDuiHandler = pParentObj->GetDuiHandler();
		if(pDuiHandler != NULL)
		{
			return pDuiHandler->OnDuiMessage(GetID(), GetName(), uMsg, wParam, lParam);
		}

		if(pParentObj->IsClass("popup"))
		{
			pParentObj = ((CDlgPopup*)pParentObj)->GetParent();
		}else
		if(pParentObj->IsClass("dlg"))	// 如果是对话框，暂时终结，不再找父窗口
		{
			pParentObj = ((CDlgBase*)pParentObj)->GetParent();
		}else
		{
			pParentObj = ((CControlBase*)pParentObj)->GetParent();
		}
	}

	// 如果未找到事件处理对象,则通过DuiSystem调用所有注册的事件处理对象进行处理
	return DuiSystem::Instance()->CallDuiHandler(GetID(), GetName(), uMsg, wParam, lParam);
}

// 发送通知消息
LRESULT CControlBase::SendMessage(UINT Msg, WPARAM wParam, LPARAM lParam)
{
	//return ::SendMessage(m_hWnd, Msg, wParam, lParam);
	return m_pParentDuiObject->OnBaseMessage(m_uID, Msg, wParam, lParam);
}


////////////////////////////////////////////////////////////////////////
// CControlBaseFont

CControlBaseFont::CControlBaseFont(HWND hWnd, CDuiObject* pDuiObject)
		: CControlBase(hWnd, pDuiObject)								  
{
	m_strTitle = _T("");
	m_strFont = DuiSystem::GetDefaultFont();
	m_nFontWidth = 12;
	m_fontStyle = FontStyleRegular;
	m_uAlignment = DT_LEFT;
	m_uVAlignment = DT_TOP;

	m_pImage = NULL;
	m_nImagePicCount = 4;
}

CControlBaseFont::CControlBaseFont(HWND hWnd, CDuiObject* pDuiObject, UINT uControlID, CRect rc, CString strTitle, BOOL bIsVisible/* = TRUE*/, BOOL bIsDisable/* = FALSE*/ , BOOL bRresponse/* = TRUE*/,
								   CString strFont, int nFontWidth/* = 12*/, FontStyle fontStyle/* = FontStyleRegular*/)
								   : CControlBase(hWnd, pDuiObject, uControlID, rc, bIsVisible, bIsDisable, bRresponse)								  
{
	m_strTitle = strTitle;
	m_strFont = DuiSystem::GetDefaultFont(strFont);
	m_nFontWidth = nFontWidth;
	m_fontStyle = fontStyle;
	m_uAlignment = DT_LEFT;
	m_uVAlignment = DT_TOP;

	m_pImage = NULL;
	m_nImagePicCount = 4;
}

CControlBaseFont::~CControlBaseFont(void)
{
	if(m_pImage != NULL)
	{
		delete m_pImage;
		m_pImage = NULL;
	}
}

// 设置title
void CControlBaseFont::SetTitle(CString strTitle) 
{
	//if(strTitle != m_strTitle)
	{
		SetControlTitle(strTitle);
		UpdateControl(true);
	}
}

// 设置水平对齐方式
void CControlBaseFont::SetAlignment(UINT uAlignment)
{
	if(uAlignment != m_uAlignment)
	{
		m_uAlignment = uAlignment;
		UpdateControl(true);
	}
}

// 设置垂直对齐方式
void CControlBaseFont::SetVAlignment(UINT uVAlignment)
{
	if(uVAlignment != m_uVAlignment)
	{
		m_uVAlignment = uVAlignment;
		UpdateControl(true);
	}
}

// 设置对齐方式
void CControlBaseFont::SetAlignment(UINT uAlignment, UINT uVAlignment)
{
	if(uAlignment != m_uAlignment || uVAlignment != m_uVAlignment)
	{
		m_uAlignment = uAlignment;
		m_uVAlignment = uVAlignment;

		UpdateControl(true);
	}
}

// 设置字体
void CControlBaseFont::SetFont(CString strFont, int nFontWidth, FontStyle fontStyle)
{
	if(m_strFont != strFont || m_nFontWidth != nFontWidth || m_fontStyle != fontStyle)
	{
		m_strFont = DuiSystem::GetDefaultFont(strFont);
		m_nFontWidth = nFontWidth;
		m_fontStyle = fontStyle;
		UpdateControl(true);
	}
}

// 设置图片
BOOL CControlBaseFont::SetBitmap(UINT nResourceID, CString strType)
{
	if(m_pImage != NULL)
	{
		delete m_pImage;
		m_pImage = NULL;
	}

	if(ImageFromIDResource(nResourceID, strType, m_pImage))
	{
		m_sizeImage.SetSize(m_pImage->GetWidth() / m_nImagePicCount, m_pImage->GetHeight());
		UpdateControl(true);
		return true;
	}
	return false;
}

// 设置图片
BOOL CControlBaseFont::SetBitmap(CString strImage)
{
	if(m_pImage != NULL)
	{
		delete m_pImage;
		m_pImage = NULL;
	}

	m_pImage = Image::FromFile(strImage, m_bImageUseECM);

	if(m_pImage->GetLastStatus() == Ok)
	{
		m_sizeImage.SetSize(m_pImage->GetWidth() / m_nImagePicCount, m_pImage->GetHeight());
		UpdateControl(true);
		return true;
	}
	return false;
}

// 设置Image中默认包含的图片个数
void CControlBaseFont::SetBitmapCount(int nCount)
{
	m_nImagePicCount = nCount;
}

// 设置图片
BOOL CControlBaseFont::SetImage(CString strImage)
{
	CStringA strImageA = CEncodingUtil::UnicodeToAnsi(strImage);
	// 通过Skin读取
	CStringA strSkin = "";
	if(strImageA.Find("skin:") == 0)
	{
		strSkin = DuiSystem::Instance()->GetSkin(strImageA);
		if (strSkin.IsEmpty()) return FALSE;
	}else
	{
		strSkin = strImageA;
	}

	if(strSkin.Find(".") != -1)	// 加载图片文件
	{
		CString strImgFile = DuiSystem::GetSkinPath() + CA2T(strSkin, CP_UTF8);
		if(strSkin.Find(":") != -1)
		{
			strImgFile = CA2T(strSkin, CP_UTF8);
		}
		if(!SetBitmap(strImgFile))
		{
			return FALSE;
		}
	}else	// 加载图片资源
	{
		UINT nResourceID = atoi(strSkin);
		if(!SetBitmap(nResourceID, TEXT("PNG")))
		{
			if(!SetBitmap(nResourceID, TEXT("BMP")))
			{
				return FALSE;
			}
		}
	}

	return TRUE;
}

// 从XML设置图片信息属性
HRESULT CControlBaseFont::OnAttributeImage(const CStringA& strValue, BOOL bLoading)
{
	if (strValue.IsEmpty()) return E_FAIL;

	// 通过Skin读取
	CStringA strSkin = "";
	if(strValue.Find("skin:") == 0)
	{
		strSkin = DuiSystem::Instance()->GetSkin(strValue);
		if (strSkin.IsEmpty()) return E_FAIL;
	}else
	{
		strSkin = strValue;
	}

	if(strSkin.Find(".") != -1)	// 加载图片文件
	{
		CString strImgFile = DuiSystem::GetSkinPath() + CA2T(strSkin, CP_UTF8);
		if(strSkin.Find(":") != -1)
		{
			strImgFile = CA2T(strSkin, CP_UTF8);
		}
		if(!SetBitmap(strImgFile))
		{
			return E_FAIL;
		}
	}else	// 加载图片资源
	{
		UINT nResourceID = atoi(strSkin);
		if(!SetBitmap(nResourceID, TEXT("PNG")))
		{
			if(!SetBitmap(nResourceID, TEXT("BMP")))
			{
				return E_FAIL;
			}
		}
	}

	return bLoading?S_FALSE:S_OK;
}

// 从XML设置Skin属性
HRESULT CControlBaseFont::OnAttributeSkin(const CStringA& strValue, BOOL bLoading)
{
	if (strValue.IsEmpty()) return E_FAIL;

	CStringA strSkin = DuiSystem::Instance()->GetSkin(strValue);
	if (strSkin.IsEmpty()) return E_FAIL;

	if(strSkin.Find(".") != -1)	// 加载图片文件
	{
		CString strImgFile = DuiSystem::GetSkinPath() + CA2T(strSkin, CP_UTF8);
		if(strSkin.Find(":") != -1)
		{
			strImgFile = CA2T(strSkin, CP_UTF8);
		}
		if(!SetBitmap(strImgFile))
		{
			return E_FAIL;
		}
	}else	// 加载图片资源
	{
		UINT nResourceID = atoi(strSkin);
		if(!SetBitmap(nResourceID, TEXT("PNG")))
		{
			if(!SetBitmap(nResourceID, TEXT("BMP")))
			{
				return E_FAIL;
			}
		}
	}

	return bLoading?S_FALSE:S_OK;
}

// 从XML设置Font属性
HRESULT CControlBaseFont::OnAttributeFont(const CStringA& strValue, BOOL bLoading)
{
	if (strValue.IsEmpty()) return E_FAIL;

	DuiFontInfo fontInfo;
	BOOL bFindFont = DuiSystem::Instance()->GetFont(strValue, fontInfo);
	if (!bFindFont) return E_FAIL;

	m_strFont = fontInfo.strFont;
	m_nFontWidth = fontInfo.nFontWidth;	
	m_fontStyle = fontInfo.fontStyle;

	return bLoading?S_FALSE:S_OK;
}