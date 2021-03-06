// ChineseChessView.cpp : CChineseChessView 类的实现
// @author: onezeros@yahoo.cn 
//	adapted from Pham Hong Nguyen

#include "stdafx.h"
#include "ChineseChess.h"

#include "ChineseChessDoc.h"
#include "ChineseChessView.h"

#include <math.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


// CChineseChessView

IMPLEMENT_DYNCREATE(CChineseChessView, CView)

BEGIN_MESSAGE_MAP(CChineseChessView, CView)
	// 标准打印命令
	ON_COMMAND(ID_FILE_PRINT, &CView::OnFilePrint)
	ON_COMMAND(ID_FILE_PRINT_DIRECT, &CView::OnFilePrint)
	ON_COMMAND(ID_FILE_PRINT_PREVIEW, &CView::OnFilePrintPreview)
	ON_WM_LBUTTONDOWN()
	ON_WM_LBUTTONUP()
	ON_WM_MOUSEMOVE()
	ON_WM_PAINT()
	ON_COMMAND(ID_FILE_NEW, &CChineseChessView::OnNewGame)
	ON_COMMAND(ID_FILE_OPEN, &CChineseChessView::OnFileOpen)
	ON_COMMAND(ID_FILE_SAVE, &CChineseChessView::OnFileSave)
	ON_WM_TIMER()
END_MESSAGE_MAP()



/* the board representation && the initial board state */
// 0, 1,represent for both sides
char color[BOARD_SIZE];

char piece[BOARD_SIZE];

/* For getting information */
unsigned long nodecount, brandtotal = 0, gencount = 0;
char ply, side, xside, computerside;
move newmove;
gen_rec gen_dat[MOVE_STACK];//record moved steps
//store possible moves indexs in gen_data for  current situation
short gen_begin[HIST_STACK], gen_end[HIST_STACK];
hist_rec hist_dat[HIST_STACK];//history data
short hdp;


/**** MOVE GENERATE ****/
const short offset[7][8] =//possible positions offset
{{-1, 1,13, 0, 0, 0, 0, 0}, /* PAWN {for DARK side} */
{-12,-14,12,14,0,0,0,0}, /* BISHOP */
{-28,-24,24,28, 0, 0, 0, 0 }, /* ELEPHAN */
{-11,-15,-25,-27,11,15,25,27}, /* KNIGHT */
{-1, 1,-13,13, 0, 0, 0, 0}, /* CANNON */
{-1, 1,-13,13, 0, 0, 0, 0}, /* ROOK */
{-1, 1,-13,13, 0, 0, 0, 0}/* KING */
}; 

const short mailbox182[182] =//14*13,10*9
{-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
-1,-1, 0, 1, 2, 3, 4, 5, 6, 7, 8,-1,-1,
-1,-1, 9,10,11,12,13,14,15,16,17,-1,-1,
-1,-1,18,19,20,21,22,23,24,25,26,-1,-1,
-1,-1,27,28,29,30,31,32,33,34,35,-1,-1,
-1,-1,36,37,38,39,40,41,42,43,44,-1,-1,
-1,-1,45,46,47,48,49,50,51,52,53,-1,-1,
-1,-1,54,55,56,57,58,59,60,61,62,-1,-1,
-1,-1,63,64,65,66,67,68,69,70,71,-1,-1,
-1,-1,72,73,74,75,76,77,78,79,80,-1,-1,
-1,-1,81,82,83,84,85,86,87,88,89,-1,-1,
-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1
};

//positions in mailbox182
const short mailbox90[90] =//10*9
{28, 29, 30, 31, 32, 33, 34, 35, 36,//+5
41, 42, 43, 44, 45, 46, 47, 48, 49,
54, 55, 56, 57, 58, 59, 60, 61, 62,
67, 68, 69, 70, 71, 72, 73, 74, 75,
80, 81, 82, 83, 84, 85, 86, 87, 88,
93, 94, 95, 96, 97, 98, 99,100,101,
106, 107,108,109,110,111,112,113,114,
119, 120,121,122,123,124,125,126,127,
132, 133,134,135,136,137,138,139,140,
145, 146,147,148,149,150,151,152,153
};

const short legalposition[90] =
{1, 1, 5, 3, 3, 3, 5, 1, 1,
1, 1, 1, 3, 3, 3, 1, 1, 1,
5, 1, 1, 3, 7, 3, 1, 1, 5,
1, 1, 1, 1, 1, 1, 1, 1, 1,
9, 1,13, 1, 9, 1,13, 1, 9,
9, 9, 9, 9, 9, 9, 9, 9, 9,
9, 9, 9, 9, 9, 9, 9, 9, 9,
9, 9, 9, 9, 9, 9, 9, 9, 9,
9, 9, 9, 9, 9, 9, 9, 9, 9,
9, 9, 9, 9, 9, 9, 9, 9, 9
};

const short maskpiece[7] = {8, 2, 4, 1, 1, 1, 2};
const short knightcheck[8] = {1,-1,-9,-9,-1,1,9,9};
const short elephancheck[8] = {-10,-8,8,10,0,0,0,0};
const short kingpalace[9] = {3,4,5,12,13,14,21,22,23};//possible positions for computer side



//check whether computer's King will be killed by opponent's King directly
// after computer moves King,
short KingFace(short from, short dest)
{
	short i, k, t, r = 0;
	i = from % SIZE_X;
	if (i>=3 && i<=5 && piece[dest]!=KING)
	{
		t = piece[dest]; piece[dest] = piece[from]; piece[from] = EMPTY;//make the move
		i = 0;
		for (k=kingpalace[i]; piece[k]!=KING; k++) ;
		for (k += SIZE_X; k<BOARD_SIZE && piece[k]==EMPTY; k += SIZE_X);
		if (piece[k]==KING) r = 1;
		piece[from] = piece[dest]; piece[dest] = t;//unmove
	}
	return r;
}
//save a possible move
void Gen_push(short from, short dest)
{
	if (!KingFace(from, dest))
	{
		gen_dat[gen_end[ply]].m.from = from;
		gen_dat[gen_end[ply]].m.dest = dest;
		gen_end[ply]++;
	}
}

//generate all possible moves
void Gen(void)
{
	short i, j, k, n, p, x, y, t, fcannon;

	gen_end[ply] = gen_begin[ply];

	for (i=0; i < BOARD_SIZE; i++){
		if (color[i]==side)
		{
			p = piece[i];//piece kind
			for (j=0; j<8; j++)
			{
				if (!offset[p][j]) break;//find possible next position
				x = mailbox90[i]; //offset in mailbox128
				fcannon = 0;
				if (p==ROOK || p==CANNON) n = 9; else n = 1;//
				for (k=0; k<n; k++)
				{
					//  get offset result for (p==PAWN && side==LIGHT)
					//there is no offset table for it
					if (p==PAWN && side==LIGHT) x -= offset[p][j]; else x += offset[p][j];

					y = mailbox182[x];
					//  t for the position in the board of this piece ,
					//according which side the piece is 
					if (side == DARK) t = y; else t = 89-y;
					if (y==-1 || (legalposition[t] & maskpiece[p])==0) break;

					if (!fcannon)
					{
						if (color[y]!=side)
							switch (p)
						{
							case KNIGHT: if (color[i+knightcheck[j]]==EMPTY) Gen_push(i, y); break;
							case ELEPHANT:if (color[i+elephancheck[j]]==EMPTY) Gen_push(i, y); break;
							case CANNON: if (color[y]==EMPTY) Gen_push(i, y); break;
							default: Gen_push(i, y);
						}
						if (color[y]!=EMPTY) { if (p==CANNON) fcannon++; else break; }
					}
					else   /* CANNON switch */
					{
						if (color[y] != EMPTY)
						{
							if (color[y]==xside) Gen_push(i, y);
							break;
						}
					}
				} /* for k */
			} /* for j */
		}
	}
	gen_end[ply+1] = gen_end[ply]; gen_begin[ply+1] = gen_end[ply];
	brandtotal += gen_end[ply] - gen_begin[ply]; gencount++;
}


/***** MOVE *****/
//virtual move
short MakeMove(move m)
{
	short from, dest, p;
	nodecount++;
	from = m.from; dest = m.dest;
	hist_dat[hdp].m = m; hist_dat[hdp].capture = p = piece[dest];
	piece[dest] = piece[from]; piece[from] = EMPTY;
	color[dest] = color[from]; color[from] = EMPTY;
	hdp++; ply++; side = xside; xside = 1-xside;
	return p == KING;
}


void UnMakeMove(void)
{
	short from, dest;
	hdp--; ply--; side = xside; xside = 1-xside;
	from = hist_dat[hdp].m.from; dest = hist_dat[hdp].m.dest;
	piece[from] = piece[dest]; color[from] = color[dest];
	piece[dest] = hist_dat[hdp].capture;
	if (piece[dest] == EMPTY) color[dest] = EMPTY; else color[dest] = xside;
}

/***** EVALUATE *****/
//  evaluate for current board simply by counting how many and 
//what kind of pieces left on the board
short Eval(void)
{
	//values for every kind of pieces
	static short piecevalue[7] = {10, 20, 20, 40, 45, 90, 1000};
	short i, s = 0;
	for (i=0; i<BOARD_SIZE; i++)
		if (color[i]==side) s += piecevalue[piece[i]];
		else if (color[i]==xside) s -= piecevalue[piece[i]];
		return s;
}


/***** SEARCH *****/
/* Search game tree by alpha-beta algorith */
short AlphaBeta(short alpha, short beta, short depth)
{
	short i, value, best;

	if (!depth) return Eval();

	Gen();
	best = -INFINITY;

	for (i=gen_begin[ply]; i<gen_end[ply] && best<beta; i++)
	{
		if (best > alpha) alpha = best;

		if (MakeMove(gen_dat[i].m)) value = 1000-ply;
		else value = -AlphaBeta(-beta, -alpha, depth-1);
		UnMakeMove();

		if (value > best)
		{
			best = value; if (!ply) newmove = gen_dat[i].m;
		}
	}

	return best;
}
//real move
short UpdateNewMove(void)
{
	short from, dest, p;
	from = newmove.from; dest = newmove.dest; p = piece[dest];
	piece[dest] = piece[from]; piece[from] = EMPTY;
	color[dest] = color[from]; color[from] = EMPTY;	
	return p == KING;
}
// CChineseChessView 构造/析构


short GetHumanMove(void)
{
	return 0;
}

CChineseChessView::CChineseChessView():isMouseDown(false),org(30,30),boardButtomDown(org.x+8*lattice_len,org.y+9*lattice_len)
{
	// TODO: 在此处添加构造代码
	//load icons
	for (int i=0;i<7;i++){
		hIcons[0][i]=NULL;
		hIcons[1][i]=NULL;
	}
	CWinApp* pApp=AfxGetApp();

	hIcons[DARK][PAWN]=pApp->LoadIcon(IDI_ICON_BLACK_PAWN);
	hIcons[DARK][BISHOP]=pApp->LoadIcon(IDI_ICON_BLACK_BISHOP);
	hIcons[DARK][ELEPHANT]=pApp->LoadIcon(IDI_ICON_BLACK_ELEPHANT);
	hIcons[DARK][KNIGHT]=pApp->LoadIcon(IDI_ICON_BLACK_KNIGHT);
	hIcons[DARK][CANNON]=pApp->LoadIcon(IDI_ICON_BLACK_CANNON);
	hIcons[DARK][ROOK]=pApp->LoadIcon(IDI_ICON_BLACK_ROOK);
	hIcons[DARK][KING]=pApp->LoadIcon(IDI_ICON_BLACK_KING);
	hIcons[LIGHT][PAWN]=pApp->LoadIcon(IDI_ICON_RED_PAWN);
	hIcons[LIGHT][BISHOP]=pApp->LoadIcon(IDI_ICON_RED_BISHOP);
	hIcons[LIGHT][ELEPHANT]=pApp->LoadIcon(IDI_ICON_RED_ELEPHANT);
	hIcons[LIGHT][KNIGHT]=pApp->LoadIcon(IDI_ICON_RED_KNIGHT);
	hIcons[LIGHT][CANNON]=pApp->LoadIcon(IDI_ICON_RED_CANNON);
	hIcons[LIGHT][ROOK]=pApp->LoadIcon(IDI_ICON_RED_ROOK);
	hIcons[LIGHT][KING]=pApp->LoadIcon(IDI_ICON_RED_KING);
	for (int i=0;i<14;i++){
		if (hIcons[i]==NULL){
			MessageBox("failed to load icon");
		}
	}	

	pDCBoard=new CDC;
	pDCBack=new CDC;

}

CChineseChessView::~CChineseChessView()
{
}

BOOL CChineseChessView::PreCreateWindow(CREATESTRUCT& cs)
{
	// TODO: 在此处通过修改
	//  CREATESTRUCT cs 来修改窗口类或样式
	
	return CView::PreCreateWindow(cs);
}

// CChineseChessView 绘制

void CChineseChessView::OnDraw(CDC* /*pDC*/)
{
	CChineseChessDoc* pDoc = GetDocument();
	ASSERT_VALID(pDoc);
	if (!pDoc)
		return;

	// TODO: 在此处为本机数据添加绘制代码
}


// CChineseChessView 打印

BOOL CChineseChessView::OnPreparePrinting(CPrintInfo* pInfo)
{
	// 默认准备

	return DoPreparePrinting(pInfo);
}

void CChineseChessView::OnBeginPrinting(CDC* /*pDC*/, CPrintInfo* /*pInfo*/)
{
	// TODO: 添加额外的打印前进行的初始化过程
}

void CChineseChessView::OnEndPrinting(CDC* /*pDC*/, CPrintInfo* /*pInfo*/)
{
	// TODO: 添加打印后进行的清理过程
}


// CChineseChessView 诊断

#ifdef _DEBUG
void CChineseChessView::AssertValid() const
{
	CView::AssertValid();
}

void CChineseChessView::Dump(CDumpContext& dc) const
{
	CView::Dump(dc);
}

CChineseChessDoc* CChineseChessView::GetDocument() const // 非调试版本是内联的
{
	ASSERT(m_pDocument->IsKindOf(RUNTIME_CLASS(CChineseChessDoc)));
	return (CChineseChessDoc*)m_pDocument;
}
#endif //_DEBUG


// CChineseChessView 消息处理程序

void CChineseChessView::OnInitialUpdate()
{
	CView::OnInitialUpdate();
	
	// TODO: ÔÚ´ËÌí¼Ó×¨ÓÃ´úÂëºÍ/»òµ÷ÓÃ»ùÀà
	NewGame(false);

	CClientDC dc(this);
	CRect rect;
	GetClientRect(&rect);

	bmpBack.CreateCompatibleBitmap(&dc,rect.Width(),rect.Height());
	bmpBoard.CreateCompatibleBitmap(&dc,rect.Width(),rect.Height());
	pDCBack->CreateCompatibleDC(&dc);
	pDCBoard->CreateCompatibleDC(&dc);
	pDCBack->SelectObject(&bmpBack);
	pDCBoard->SelectObject(&bmpBoard);

	CBrush br(GetSysColor(COLOR_3DFACE));	
	pDCBoard->FillRect(rect,&br);
	
	CPen pen[2];
	pen[0].CreatePen(PS_SOLID,0,GetSysColor(COLOR_3DHILIGHT));
	pen[1].CreatePen(PS_SOLID,0,GetSysColor(COLOR_3DSHADOW));
	
	//CPen pen;
	//pen.CreatePen(PS_SOLID,2,RGB(0,0,0));	
	//draw board grid
	//draw chess board	
	for (int i=0;i<2;i++){
		pDCBoard->SelectObject(pen+i);
		for (int i=0;i<10;i++){
			pDCBoard->MoveTo(org.x,org.y+i*lattice_len);
			pDCBoard->LineTo(org.x+8*lattice_len,org.y+i*lattice_len);
		}
		pDCBoard->MoveTo(org);
		pDCBoard->LineTo(org.x,org.y+9*lattice_len);
		pDCBoard->MoveTo(org.x+8*lattice_len,org.y);
		pDCBoard->LineTo(org.x+8*lattice_len,org.y+9*lattice_len);
		for (int i=1;i<=7;i++){
			pDCBoard->MoveTo(org.x+i*lattice_len,org.y);
			pDCBoard->LineTo(org.x+i*lattice_len,org.y+4*lattice_len);
			pDCBoard->MoveTo(org.x+i*lattice_len,org.y+5*lattice_len);
			pDCBoard->LineTo(org.x+i*lattice_len,org.y+9*lattice_len);
		}
		//cross line
		pDCBoard->MoveTo(org.x+3*lattice_len,org.y);
		pDCBoard->LineTo(org.x+5*lattice_len,org.y+2*lattice_len);
		pDCBoard->MoveTo(org.x+5*lattice_len,org.y);
		pDCBoard->LineTo(org.x+3*lattice_len,org.y+2*lattice_len);
		pDCBoard->MoveTo(org.x+3*lattice_len,org.y+7*lattice_len);
		pDCBoard->LineTo(org.x+5*lattice_len,org.y+9*lattice_len);
		pDCBoard->MoveTo(org.x+5*lattice_len,org.y+7*lattice_len);
		pDCBoard->LineTo(org.x+3*lattice_len,org.y+9*lattice_len);
		//add a surrounding boarder
		static int boarder_interval=3;
		pDCBoard->MoveTo(org.x-boarder_interval,org.y-boarder_interval);
		pDCBoard->LineTo(org.x+8*lattice_len+boarder_interval,org.y-boarder_interval);
		pDCBoard->LineTo(org.x+8*lattice_len+boarder_interval,org.y+9*lattice_len+boarder_interval);
		pDCBoard->LineTo(org.x-boarder_interval,org.y+9*lattice_len+boarder_interval);
		pDCBoard->LineTo(org.x-boarder_interval,org.y-boarder_interval);
	}
		

}

void CChineseChessView::OnLButtonDown(UINT nFlags, CPoint point)
{
	// TODO: ÔÚ´ËÌí¼ÓÏûÏ¢´¦Àí³ÌÐò´úÂëºÍ/»òµ÷ÓÃÄ¬ÈÏÖµ
	CPoint logpoint=PhysicalToLogicPoint(point);
	if (logpoint.x>=0){
		int num=LogicPointToNum(logpoint);
		if (color[num]==LIGHT){
			selectedPoint=logpoint;
			isMouseDown=true;
			pieceDragPoint=LogicToPhysicPoint(logpoint);
			pieceDragPoint.x-=icon_len_half;
			pieceDragPoint.y-=icon_len_half;
			lastMousePoint=point;
		}
	}
	CView::OnLButtonDown(nFlags, point);
}

void CChineseChessView::OnLButtonUp(UINT nFlags, CPoint point)
{
	// TODO: ÔÚ´ËÌí¼ÓÏûÏ¢´¦Àí³ÌÐò´úÂëºÍ/»òµ÷ÓÃÄ¬ÈÏÖµ
	if (isMouseDown){
		isMouseDown=false;		
		CRect rect;
		GetClientRect(&rect);

		CPoint logpoint=PhysicalToLogicPoint(point);
		if (logpoint.x>=0){	
			//human move
			Gen();
			newmove.from = LogicPointToNum(selectedPoint); 
			newmove.dest = LogicPointToNum(logpoint);
			for (int i=gen_begin[ply]; i<gen_end[ply]; i++){
				if (gen_dat[i].m.from==newmove.from && gen_dat[i].m.dest==newmove.dest){
					if(UpdateNewMove()){
						UpdateDisplay(rect);
						int ret;
						ret=MessageBox("you are really a lucky dog,dare to try again?","Game Over",MB_YESNO);
						if (ret==IDYES){
							NewGame(true);
							return;
						}else{
							exit(0);
						}
					}
					side = xside; xside = 1-xside;
					//computer move
					short best;			
					best = AlphaBeta(-INFINITY, INFINITY, MAX_PLY);
					if(UpdateNewMove()){
						UpdateDisplay(rect);
						int ret;
						ret=MessageBox("afraid?dare to try again?","Game Over",MB_YESNO);
						if (ret==IDYES){
							NewGame(true);
							return;
						}else{
							exit(0);
						}
					}
					side = xside; xside = 1-xside;
					break;
				}
			}
			
		}

		UpdateDisplay(rect);
		
	}
	CView::OnLButtonUp(nFlags, point);
}

void CChineseChessView::OnMouseMove(UINT nFlags, CPoint point)
{
	// TODO: ÔÚ´ËÌí¼ÓÏûÏ¢´¦Àí³ÌÐò´úÂëºÍ/»òµ÷ÓÃÄ¬ÈÏÖµ
	if (isMouseDown){
		CPoint dragPoint=pieceDragPoint+point-lastMousePoint;
		CRect rc;
		if (dragPoint.x>=pieceDragPoint.x){
			rc.right=dragPoint.x+icon_len;
			rc.left=pieceDragPoint.x;
		}else{
			rc.right=pieceDragPoint.x+icon_len;
			rc.left=dragPoint.x;
		}
		if (dragPoint.y>=pieceDragPoint.y){
			rc.bottom=dragPoint.y+icon_len;
			rc.top=pieceDragPoint.y;
		}else{
			rc.bottom=pieceDragPoint.y+icon_len;
			rc.top=dragPoint.y;
		}
		pieceDragPoint=dragPoint;
		lastMousePoint=point;
		UpdateDisplay(rc);
	}
	
	CView::OnMouseMove(nFlags, point);
}

void CChineseChessView::OnPaint()
{
	CPaintDC dc(this); // device context for painting

	CRect rc;
	GetClientRect(&rc);
	//GetUpdateRect(&rc,TRUE);	
	UpdateDisplay(rc);
}

void CChineseChessView::UpdateDisplay(CRect& rc)
{
	pDCBack->BitBlt(rc.left,rc.top,rc.Width(),rc.Height(),	pDCBoard,rc.left,rc.top,SRCCOPY);

	CPoint align(icon_len_half,icon_len_half);
	CPoint logPoint;
	for (int i=0;i<90;i++){
		if(piece[i]!=EMPTY){
			logPoint=NumToLogicPoint(i);
			if (!isMouseDown||isMouseDown&&logPoint!=selectedPoint){
				CSize sz=LogicToPhysicPoint(logPoint)-align;
				pDCBack->DrawIcon(CPoint(sz.cx,sz.cy),hIcons[color[i]][piece[i]]);
			}
		}
	}
	//draw the selected point
	if (isMouseDown){
		pDCBack->DrawIcon(pieceDragPoint,hIcons[side][piece[selectedPoint.y*9+selectedPoint.x]]);
	}	

	CClientDC dc(this);
	dc.BitBlt(rc.left,rc.top,rc.Width(),rc.Height(),pDCBack,rc.left,rc.top,SRCCOPY);
}

inline CPoint CChineseChessView::NumToLogicPoint(int n)
{
	return CPoint(n%9,n/9);
}
inline int CChineseChessView::LogicPointToNum(CPoint& p)
{	
	return p.x+p.y*9;
}

inline CPoint CChineseChessView::LogicToPhysicPoint(CPoint p)
{
	p.x*=lattice_len;
	p.y*=lattice_len;
	p+=org;
	return p;
}
inline double distance(CPoint& p1,CPoint&p2){
	return (p1.x-p2.x)*(p1.x-p2.x)+(p1.y-p2.y)*(p1.y-p2.y);
}
CPoint CChineseChessView::PhysicalToLogicPoint(CPoint p)
{
	//could be covered by chess
	CPoint result(-1,-1);	
	if (p.x<org.x-icon_len_half||p.y<org.y-icon_len_half||
		p.x>boardButtomDown.x+icon_len_half||p.y>boardButtomDown.y+icon_len_half){
			return result;
	}
	p-=org;
	result.x=p.x/lattice_len;
	result.y=p.y/lattice_len;
	int remainder_x=p.x%lattice_len;
	int remainder_y=p.y%lattice_len;

	if (remainder_x>0&&
		remainder_x>lattice_len-icon_len_half){
			result.x++;
	}
	if (remainder_y>0&&
		remainder_y>lattice_len-icon_len_half){
			result.y++;
	}
	p+=org;
	CPoint tp=LogicToPhysicPoint(result);
	if (distance(tp,p)<=icon_len_half*icon_len_half){
		return result;
	}

	return CPoint(-1,-1);
}


void CChineseChessView::NewGame(bool b)
{
	gen_begin[0] = 0; 
	ply = 0; 
	hdp = 0;
	side = LIGHT; 
	xside = DARK; 
	computerside = DARK;

	char clr[BOARD_SIZE]={
		0, 0, 0, 0, 0, 0, 0, 0, 0,
		7, 7, 7, 7, 7, 7, 7, 7, 7,
		7, 0, 7, 7, 7, 7, 7, 0, 7,
		0, 7, 0, 7, 0, 7, 0, 7, 0,
		7, 7, 7, 7, 7, 7, 7, 7, 7,
		7, 7, 7, 7, 7, 7, 7, 7, 7,
		1, 7, 1, 7, 1, 7, 1, 7, 1,
		7, 1, 7, 7, 7, 7, 7, 1, 7,
		7, 7, 7, 7, 7, 7, 7, 7, 7,
		1, 1, 1, 1, 1, 1, 1, 1, 1
	};
	char pc[BOARD_SIZE]=
	{
		5, 3, 2, 1, 6, 1, 2, 3, 5,
		7, 7, 7, 7, 7, 7, 7, 7, 7,
		7, 4, 7, 7, 7, 7, 7, 4, 7,
		0, 7, 0, 7, 0, 7, 0, 7, 0,
		7, 7, 7, 7, 7, 7, 7, 7, 7,
		7, 7, 7, 7, 7, 7, 7, 7, 7,
		0, 7, 0, 7, 0, 7, 0, 7, 0,
		7, 4, 7, 7, 7, 7, 7, 4, 7,
		7, 7, 7, 7, 7, 7, 7, 7, 7,
		5, 3, 2, 1, 6, 1, 2, 3, 5
	};
	memcpy(color,clr,BOARD_SIZE);
	memcpy(piece,pc,BOARD_SIZE);

	if (b){
		CRect rect;
		GetClientRect(&rect);
		UpdateDisplay(rect);
	}	
}

void CChineseChessView::OnNewGame()
{
	// TODO: ÔÚ´ËÌí¼ÓÃüÁî´¦Àí³ÌÐò´úÂë
	NewGame(true);
}

void CChineseChessView::OnFileOpen()
{
	// TODO: ÔÚ´ËÌí¼ÓÃüÁî´¦Àí³ÌÐò´úÂë	
	CFileDialog fileDlg(true);
	CString filename;
	if (fileDlg.DoModal()==IDOK){
		filename=fileDlg.GetPathName();
		CFile file;
		file.Open(filename.GetBuffer(filename.GetLength()),CFile::modeRead);
		if (!file){
			MessageBox("failed to open file ");
			return;
		}
		char logo[6];
		file.Read(logo,5);
		logo[5]='\0';
		if (!strcmp(logo,"chess")){
			file.Read(color,BOARD_SIZE);
			file.Read(piece,BOARD_SIZE);
			file.Read(&nodecount,sizeof(unsigned long));
			file.Read(&brandtotal,sizeof(unsigned long));
			file.Read(&gencount,sizeof(unsigned long));
			file.Read(&ply,1);
			file.Read(&side,1);
			file.Read(&xside,1);
			file.Read(&computerside,1);
			file.Read(&newmove,sizeof(move));
			file.Read(gen_dat,sizeof(gen_rec)*MOVE_STACK);
			file.Read(gen_begin,sizeof(short)*HIST_STACK);
			file.Read(gen_end,sizeof(short)*HIST_STACK);
			file.Read(hist_dat,sizeof(hist_rec)*HIST_STACK);
			file.Read(&hdp,sizeof(short));

			CRect rect;
			GetClientRect(&rect);
			UpdateDisplay(rect);
		}else{
			MessageBox("it's not a config file");
		}

		file.Close();
	}
}

void CChineseChessView::OnFileSave()
{
	// TODO: 
	CFileDialog fileDlg(false);
	CString filename;
	if (fileDlg.DoModal()==IDOK){
		filename=fileDlg.GetPathName();
		CFile file;
		file.Open(filename.GetBuffer(filename.GetLength()),CFile::modeCreate|CFile::modeWrite);
		if (!file){
			MessageBox("failed to open file ");			
		}else{
			char logo[6]="chess";
			file.Write(logo,5);
			file.Write(color,BOARD_SIZE);
			file.Write(piece,BOARD_SIZE);
			file.Write(&nodecount,sizeof(unsigned long));
			file.Write(&brandtotal,sizeof(unsigned long));
			file.Write(&gencount,sizeof(unsigned long));
			file.Write(&ply,1);
			file.Write(&side,1);
			file.Write(&xside,1);
			file.Write(&computerside,1);
			file.Write(&newmove,sizeof(move));
			file.Write(gen_dat,sizeof(gen_rec)*MOVE_STACK);
			file.Write(gen_begin,sizeof(short)*HIST_STACK);
			file.Write(gen_end,sizeof(short)*HIST_STACK);
			file.Write(hist_dat,sizeof(hist_rec)*HIST_STACK);
			file.Write(&hdp,sizeof(short));

			file.Close();

			CRect rect;
			GetClientRect(&rect);
			UpdateDisplay(rect);
		}
	}
}

void CChineseChessView::OnTimer(UINT_PTR nIDEvent)
{
	// TODO: 

	CView::OnTimer(nIDEvent);
}
