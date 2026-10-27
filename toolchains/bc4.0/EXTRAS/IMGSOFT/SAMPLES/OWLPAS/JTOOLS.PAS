{*-----------------------------------------------------------------
 * jtools.pas - Tool Bar Unit													 
 * 1/18/93 - 9:26:54am																	 
 * John Doe																				 
 *-----------------------------------------------------------------}
unit JTools;
interface

uses WinTypes, WinProcs, Objects, OMemory, OWindows, Strings;

const
		maxtools = 50;		{ max number of tools - can be increased }

type
  PToolItem = ^TToolItem;
  TToolItem = object
  	nID : Integer;
  	hBitmap1, hBitmap2 : HBITMAP;
  	bState, bButton, bBorder, bShadow, bEnabled : Boolean;
  	rect : TRect;
	constructor Init(AParent: PWindowsObject; pType: PChar; id, X, Y, W, H : Integer;
			pBitmap1, pBitmap2, Shadow, Border : PChar);
	destructor Done;
  	procedure Show(PaintDC : hDC; hButtonBrush : HBRUSH; hShadowPen : HPEN);
  	procedure SetState(bNewState : Boolean);
  	function GetState: Boolean;
  	procedure Enable(bFlag : Boolean);
  	function HitTest(nX, nY : Integer): Boolean;
  	function GetID: Integer;
	end;


type
  PToolBar = ^TToolBar;
  TToolBar = object(TWindow)
	hShadowPen : HPEN;
  	hButtonBrush : HBRUSH;
  	bButtonDown : Boolean;
  	SelToolItem, NumTools : Integer;
	ToolItems : array[0..maxtools] of PToolItem;

	constructor Init(AParent: PWindowsObject; nHeight : Integer);
	destructor Done;virtual;
	procedure GetWindowClass(var AWndClass: TWndClass);virtual;
	function GetClassName: PChar;virtual;
	procedure AddToolItem(AParent: PWindowsObject; pType: PChar; id, X, Y, W, H : Integer;
		pBitmap1, pBitmap2, Shadow, Border : PChar);
	procedure SetItemState(ID : Integer; bState : Boolean);
	procedure Paint(DC : hDC; var PS : TPaintStruct);virtual;
	procedure WMLButtonDown(var Msg: TMessage);
		virtual wm_First + wm_LButtonDown;
	procedure WMLButtonUp(var Msg: TMessage);
		virtual wm_First + wm_LButtonUp;
	procedure WMMouseMove(var Msg: TMessage);
		virtual wm_First + wm_MouseMove;
  end;

implementation

constructor TToolItem.Init(AParent: PWindowsObject; pType: PChar; id, X, Y, W, H : Integer;
			pBitmap1, pBitmap2, Shadow, Border : PChar);
begin
	nID := id;
	hBitmap1 := LoadBitmap(HInstance, pBitmap1);
	hBitmap2 := LoadBitmap(HInstance, pBitmap2);

	rect.left := X;
	rect.top := Y;
	rect.right := X + W;
	rect.bottom := Y + H;
	bState := False;
	bEnabled := True;

	if Shadow^ = 'Y' then	bShadow := True else bShadow := False;

	if Border^ = 'Y' then bBorder := True else bBorder := False;

	if StrIComp(pType, 'Button') = 0 then bButton := True else	bButton := False;
end;

destructor TToolItem.Done;
begin
	if hBitmap1 > 0 then DeleteObject(hBitmap1);
	if hBitmap2 > 0 then DeleteObject(hBitmap2);
end;

function TToolItem.GetID: Integer;
begin GetID := nID; end;

procedure TToolItem.Show(PaintDC : hDC; hButtonBrush : HBRUSH; hShadowPen : HPEN);
var	MemoryDC : HDC;
	OldBitmapHandle : WORD;
	dwMode : Longint;
	hOldPen : HPEN;
	hOldBrush : HBRUSH;
	nOffset, nShift : Integer;
begin


	hOldPen := SelectObject(PaintDC, GetStockObject(BLACK_PEN));
	hOldBrush := SelectObject(PaintDC, hButtonBrush);

	nOffset := 0; nShift := 0;

	if bBorder then nOffset := nOffset+1;
	if bShadow then nOffset := nOffset+1;
	if bState and bShadow then nShift:=1;
	if bEnabled then dwMode := SRCCOPY else dwMode := MERGECOPY;

	if bBorder then Rectangle(PaintDC, rect.left, rect.top, rect.right, rect.bottom)
		else FillRect(PaintDC, rect, hButtonBrush);

	if  hBitmap1 = 0 then exit;

	MemoryDC := CreateCompatibleDC(PaintDC);
	if bState and (hBitmap2 > 0) then OldBitmapHandle := SelectObject(MemoryDC, hBitmap2)
		else  OldBitmapHandle := SelectObject(MemoryDC, hBitmap1);

	BitBlt(PaintDC, rect.left+nOffset+nShift, rect.top+nOffset+nShift, rect.right-rect.left, rect.bottom-rect.top,
        	MemoryDC, 0, 0, dwMode);

	SelectObject(MemoryDC, OldBitmapHandle);
	DeleteDC(MemoryDC);

	if bShadow then
   	begin
		if bState then SelectObject(PaintDC, hShadowPen)
        	 	else SelectObject(PaintDC, GetStockObject(WHITE_PEN));

	 	MoveTo(PaintDC, rect.left+nOffset-1, rect.bottom-nOffset);
	 	LineTo(PaintDC, rect.left+nOffset-1, rect.top+nOffset-1);
 		LineTo(PaintDC, rect.right-nOffset+1, rect.top+nOffset-1);

		if bState = False then
	      	begin
			SelectObject(PaintDC, hShadowPen);
	 		MoveTo(PaintDC, rect.right-nOffset, rect.top+nOffset-1);
			LineTo(PaintDC, rect.right-nOffset, rect.bottom-nOffset);
	 		LineTo(PaintDC, rect.left+nOffset-2, rect.bottom-nOffset);
			MoveTo(PaintDC, rect.right-nOffset-1, rect.top+nOffset);
	 		LineTo(PaintDC, rect.right-nOffset-1, rect.bottom-nOffset-1);
			LineTo(PaintDC, rect.left+nOffset-1, rect.bottom-nOffset-1);
		end;
	end;

	SelectObject(PaintDC, hOldPen);
	SelectObject(PaintDC, hOldBrush);
end;

function TToolItem.HitTest(nX,nY : Integer): Boolean;
var pt : TPOINT;
begin
	pt.x := nX; pt.y := nY;
	if not bEnabled then begin HitTest := False; exit; end;
	HitTest := PtInRect(rect, pt);
end;
function TToolItem.GetState: Boolean;
begin
 	GetState := bState;
end;
procedure TToolItem.SetState(bNewState : Boolean);
begin
	bState := bNewState;
end;
procedure TToolItem.Enable(bFlag : Boolean);
begin
	bEnabled := bFlag;
end;

constructor TToolBar.Init(AParent: PWindowsObject; nHeight : Integer);
begin
	TWindow.Init(AParent, '');
	bButtonDown := False;
	hShadowPen := CreatePen(PS_SOLID, 1, GetSysColor(COLOR_BTNSHADOW));
	hButtonBrush := GetStockObject(LTGRAY_BRUSH);
	SelToolItem := -1;	{ No tool selected }
	NumTools := 0;			{ Incremented when tools are added }
	Attr.X := 0;
	Attr.Y := 0;
	Attr.W := GetSystemMetrics(SM_CXSCREEN);	 { Default to largest possible }
	Attr.H := nHeight;
	Attr.Style := WS_CHILD or WS_VISIBLE;
	SetFlags(wb_MDIChild, False); { Make sure not an MDI Child Window}
end;

destructor TToolBar.Done;
var i : Integer;
begin
	DeleteObject(hShadowPen);
	for i := 0 to NumTools-1 do		{ Clean up tools}
		ToolItems[i]^.Done;
end;

function TToolBar.GetClassName: PChar;
begin
	GetClassName := 'ToolBar';
end;

procedure TToolBar.GetWindowClass(var AWndClass: TWndClass);
begin
	TWindow.GetWindowClass(AWndClass); { Get the default class }
	AWndClass.hbrBackground := hButtonBrush;
end;

procedure TToolBar.AddToolItem(AParent: PWindowsObject; pType: PChar; id, X, Y, W, H : Integer;
	pBitmap1, pBitmap2, Shadow, Border : PChar);
begin
	ToolItems[NumTools] := New(PToolItem, Init(AParent, pType, id, X, Y, W, H,
		pBitmap1, pBitmap2, Shadow, Border));

	NumTools := NumTools + 1;
end;

procedure TToolBar.Paint(DC : HDC; var PS : TPaintStruct);
var 	rcWin : TRect;
	hOldPen : HPEN;
	i : Integer;
begin
 	GetClientRect( HWindow, rcWin );

	hOldPen := SelectObject(DC, GetStockObject(BLACK_PEN));
	MoveTo(DC, 0, rcWin.bottom-1); LineTo(DC, rcWin.right, rcWin.bottom-1);

	SelectObject(DC, hShadowPen);
	MoveTo(DC, 0, rcWin.bottom-2); LineTo(DC, rcWin.right, rcWin.bottom-2);

	SelectObject(DC, hOldPen);

	for i := 0 to NumTools-1 do
	begin
	 	ToolItems[i]^.Show(DC, hButtonBrush, hShadowPen);
	end;
end;

procedure TToolBar.WMLButtonDown(var Msg: TMessage);
var 	i : Integer;
	DC : HDC;
begin
	SelToolItem := -1;
	for i := 0 to NumTools-1 do
	begin
		if ToolItems[i]^.HitTest(Msg.LParamLo, Msg.LParamHi) then
		begin
			SelToolItem := i;	 { Save selected tool }
			ToolItems[i]^.SetState( not ToolItems[i]^.GetState );
			DC := GetDC(HWindow);
			ToolItems[i]^.Show(DC, hButtonBrush, hShadowPen);
			ReleaseDC(HWindow, DC);

			if not ToolItems[i]^.bButton then { Tell Toolbar the CheckBox has been set }
			begin
				PostMessage(Parent^.HWindow, WM_COMMAND, ToolItems[i]^.GetID, 0);
				exit;
			end;
		end;
	end;

	bButtonDown := True;
	SetCapture(HWindow);
end;
procedure TToolBar.WMMouseMove(var Msg: TMessage);
var DC : HDC;
begin
	if SelToolItem >= 0 then
		if  bButtonDown and ToolItems[SelToolItem]^.bButton then
			if  ToolItems[SelToolItem]^.HitTest(Msg.LParamLo, Msg.LParamHi) <>
				ToolItems[SelToolItem]^.GetState then
			begin
				ToolItems[SelToolItem]^.SetState( not ToolItems[SelToolItem]^.GetState );
				DC := GetDC(HWindow);
				ToolItems[SelToolItem]^.Show(DC, hButtonBrush, hShadowPen);
				ReleaseDC(HWindow, DC);
			end;
end;
procedure TToolBar.WMLButtonUp(var Msg: TMessage);
var 	i : Integer;
	DC : HDC;
begin
	for i := 0 to NumTools-1 do
		if ToolItems[i]^.HitTest(Msg.LParamLo, Msg.LParamHi)
			and ToolItems[i]^.GetState then
		begin
			if ToolItems[i]^.bButton then
			begin
				ToolItems[i]^.SetState( not ToolItems[i]^.GetState );
				DC := GetDC(HWindow);
				ToolItems[i]^.Show(DC, hButtonBrush, hShadowPen);
				ReleaseDC(HWindow, DC);
				{ Tell Toolbar the button has been set }
				PostMessage(Parent^.HWindow, WM_COMMAND,ToolItems[i]^.GetID, 0);
			end;
		end
		else
			if ToolItems[i]^.bButton and ToolItems[i]^.GetState then
			begin
				ToolItems[i]^.SetState( not ToolItems[i]^.GetState );
				DC := GetDC(HWindow);
				ToolItems[i]^.Show(DC, hButtonBrush, hShadowPen);
				ReleaseDC(HWindow, DC);
			end;

	bButtonDown := False;
	ReleaseCapture;
end;

procedure TToolBar.SetItemState(ID : Integer; bState : Boolean);
var i : Integer;
begin
	for i := 0 to NumTools-1 do
		if ToolItems[i]^.GetID = ID then
		begin
			ToolItems[i]^.SetState(bState);
			exit;
		end;
end;

end.	{End of implementation }
