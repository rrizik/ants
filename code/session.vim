let SessionLoad = 1
if &cp | set nocp | endif
let s:cpo_save=&cpo
set cpo&vim
inoremap <silent> <C-J> :wa
inoremap <silent> <C-K> :wa:AsyncRun -mode=async ..\misc\build.bat
imap <C-V> "+pa
map! <S-Insert> *
nnoremap <silent>  :set paste!
nnoremap 	 :NERDTreeToggle
nnoremap <silent> <NL> :wa
nnoremap <silent>  :wa:AsyncRun -mode=async ..\misc\build.bat
nnoremap <silent>  :call NextQuickFixError()
map  <Plug>(ctrlp)
nnoremap <silent> b :split
nnoremap <silent> v :vsplit
vmap  "*d
map / <Plug>(incsearch-forward)
map ? <Plug>(incsearch-backward)
nmap [l <Plug>(QuickFixCurrentNumberLPrev)
nmap [q <Plug>(QuickFixCurrentNumberQPrev)
nmap ]l <Plug>(QuickFixCurrentNumberLNext)
nmap ]q <Plug>(QuickFixCurrentNumberQNext)
vmap gx <Plug>NetrwBrowseXVis
nmap gx <Plug>NetrwBrowseX
nmap g]l <Plug>(QuickFixCurrentNumberLLast)
nmap g[l <Plug>(QuickFixCurrentNumberLFirst)
nmap g]q <Plug>(QuickFixCurrentNumberQLast)
nmap g[q <Plug>(QuickFixCurrentNumberQFirst)
nmap g<C-Q> <Plug>(QuickFixCurrentNumberGo)
nmap g <Plug>(QuickFixCurrentNumberGo)
map g/ <Plug>(incsearch-stay)
nnoremap j gj
nnoremap k gk
vnoremap <silent> <Plug>NetrwBrowseXVis :call netrw#BrowseXVis()
nnoremap <silent> <Plug>NetrwBrowseX :call netrw#BrowseX(netrw#GX(),netrw#CheckIfRemote(netrw#GX()))
nnoremap <silent> <Plug>(QuickFixCurrentNumberLLast) :if ! QuickFixCurrentNumber#Border(v:count1, 1, 1)|execute "normal! \<C-\>\<C-n>\<Esc>"|if ingo#err#IsSet()|echoerr ingo#err#Get()|endif|endif
nnoremap <silent> <Plug>(QuickFixCurrentNumberLFirst) :if ! QuickFixCurrentNumber#Border(v:count1, 1, 0)|execute "normal! \<C-\>\<C-n>\<Esc>"|if ingo#err#IsSet()|echoerr ingo#err#Get()|endif|endif
nnoremap <silent> <Plug>(QuickFixCurrentNumberQLast) :if ! QuickFixCurrentNumber#Border(v:count1, 0, 1)|execute "normal! \<C-\>\<C-n>\<Esc>"|if ingo#err#IsSet()|echoerr ingo#err#Get()|endif|endif
nnoremap <silent> <Plug>(QuickFixCurrentNumberQFirst) :if ! QuickFixCurrentNumber#Border(v:count1, 0, 0)|execute "normal! \<C-\>\<C-n>\<Esc>"|if ingo#err#IsSet()|echoerr ingo#err#Get()|endif|endif
nnoremap <silent> <Plug>(QuickFixCurrentNumberLPrev) :if ! QuickFixCurrentNumber#Next(v:count1, 1, 1)|execute "normal! \<C-\>\<C-n>\<Esc>"|if ingo#err#IsSet()|echoerr ingo#err#Get()|endif|endif
nnoremap <silent> <Plug>(QuickFixCurrentNumberLNext) :if ! QuickFixCurrentNumber#Next(v:count1, 1, 0)|execute "normal! \<C-\>\<C-n>\<Esc>"|if ingo#err#IsSet()|echoerr ingo#err#Get()|endif|endif
nnoremap <silent> <Plug>(QuickFixCurrentNumberQPrev) :if ! QuickFixCurrentNumber#Next(v:count1, 0, 1)|execute "normal! \<C-\>\<C-n>\<Esc>"|if ingo#err#IsSet()|echoerr ingo#err#Get()|endif|endif
nnoremap <silent> <Plug>(QuickFixCurrentNumberQNext) :if ! QuickFixCurrentNumber#Next(v:count1, 0, 0)|execute "normal! \<C-\>\<C-n>\<Esc>"|if ingo#err#IsSet()|echoerr ingo#err#Get()|endif|endif
nnoremap <silent> <Plug>(QuickFixCurrentNumberGo) :if ! QuickFixCurrentNumber#Go(0, 1)|execute "normal! \<C-\>\<C-n>\<Esc>"|if ingo#err#IsSet()|echoerr ingo#err#Get()|endif|endif
noremap <Plug>(_incsearch-g#) g#
noremap <Plug>(_incsearch-g*) g*
noremap <Plug>(_incsearch-#) #
noremap <Plug>(_incsearch-*) *
noremap <expr> <Plug>(_incsearch-N) g:incsearch#consistent_n_direction && !v:searchforward ? 'n' : 'N'
noremap <expr> <Plug>(_incsearch-n) g:incsearch#consistent_n_direction && !v:searchforward ? 'N' : 'n'
map <Plug>(incsearch-nohl-g#) <Plug>(incsearch-nohl)<Plug>(_incsearch-g#)
map <Plug>(incsearch-nohl-g*) <Plug>(incsearch-nohl)<Plug>(_incsearch-g*)
map <Plug>(incsearch-nohl-#) <Plug>(incsearch-nohl)<Plug>(_incsearch-#)
map <Plug>(incsearch-nohl-*) <Plug>(incsearch-nohl)<Plug>(_incsearch-*)
map <Plug>(incsearch-nohl-N) <Plug>(incsearch-nohl)<Plug>(_incsearch-N)
map <Plug>(incsearch-nohl-n) <Plug>(incsearch-nohl)<Plug>(_incsearch-n)
noremap <expr> <Plug>(incsearch-nohl2) incsearch#autocmd#auto_nohlsearch(2)
noremap <expr> <Plug>(incsearch-nohl0) incsearch#autocmd#auto_nohlsearch(0)
noremap <expr> <Plug>(incsearch-nohl) incsearch#autocmd#auto_nohlsearch(1)
noremap <silent> <expr> <Plug>(incsearch-stay) incsearch#go({'command': '/', 'is_stay': 1})
noremap <silent> <expr> <Plug>(incsearch-backward) incsearch#go({'command': '?'})
noremap <silent> <expr> <Plug>(incsearch-forward) incsearch#go({'command': '/'})
map <C-P> <Plug>(ctrlp)
nnoremap <silent> <Plug>(ctrlp) :CtrlP
nnoremap <silent> <F2> :w:source $HOME/_vimrc
nnoremap <silent> <C-H> :set paste!
nnoremap <silent> <C-W>b :split
nnoremap <silent> <C-W>v :vsplit
nnoremap <silent> <C-J> :wa
nnoremap <silent> <C-K> :wa:AsyncRun -mode=async ..\misc\build.bat
nnoremap <silent> <C-N> :call NextQuickFixError()
nnoremap <silent> <F5> :wa:AsyncRun -mode=term -pos=external -focus=0 -silent ..\misc\build.bat
vmap <F4> "+yi
vmap <C-X> "*d
vmap <C-Del> "*d
vmap <S-Del> "*d
vmap <C-Insert> "*y
vmap <S-Insert> "-d"*P
nmap <S-Insert> "*P
inoremap <silent> <NL> :wa
inoremap <silent>  :wa:AsyncRun -mode=async ..\misc\build.bat
imap  "+pa
let &cpo=s:cpo_save
unlet s:cpo_save
set autoindent
set background=dark
set backspace=indent,eol,start
set belloff=all
set directory=~/vimfiles/swapfiles/
set errorformat=\ %#%f(%l\\,%c):\ %m
set expandtab
set guicursor=n-v-c:block-Cursor/lCursor,ve:ver35-Cursor,o:hor50-Cursor,i-ci:ver25-Cursor/lCursor,r-cr:hor20-Cursor/lCursor,sm:block-Cursor-blinkwait175-blinkoff150-blinkon175,a:blinkon0
set guifont=consolas:h11
set guioptions=g
set helplang=En
set incsearch
set laststatus=2
set makeprg=build.bat
set ruler
set runtimepath=~/vimfiles,~\\vimfiles\\bundle\\Vundle.vim,~\\vimfiles\\bundle\\ctrlp.vim,~\\vimfiles\\bundle\\nerdtree,~\\vimfiles\\bundle\\asyncrun.vim,~\\vimfiles\\bundle\\incsearch.vim,~\\vimfiles\\bundle\\QuickFixCurrentNumber,~\\vimfiles\\bundle\\QFEnter,C:\\Program\ Files\ (x86)\\Vim/vimfiles,C:\\Program\ Files\ (x86)\\Vim\\vim82,C:\\Program\ Files\ (x86)\\Vim/vimfiles/after,~/vimfiles/after,~/vimfiles/bundle/Vundle.vim,~/vimfiles/colors,~\\vimfiles\\bundle\\Vundle.vim/after,~\\vimfiles\\bundle\\ctrlp.vim/after,~\\vimfiles\\bundle\\nerdtree/after,~\\vimfiles\\bundle\\asyncrun.vim/after,~\\vimfiles\\bundle\\incsearch.vim/after,~\\vimfiles\\bundle\\QuickFixCurrentNumber/after,~\\vimfiles\\bundle\\QFEnter/after
set shiftwidth=4
set sidescroll=4
set sidescrolloff=4
set smartindent
set softtabstop=4
set splitbelow
set splitright
set tabpagemax=30
set tabstop=4
set window=52
let s:so_save = &so | let s:siso_save = &siso | set so=0 siso=0
let v:this_session=expand("<sfile>:p")
silent only
silent tabonly
cd C:\sh1tz\apesticks\cc++\ants\code
if expand('%') == '' && !&modified && line('$') <= 1 && getline(1) == ''
  let s:wipebuf = bufnr('%')
endif
set shortmess=aoO
argglobal
%argdel
set stal=2
tabnew
tabrewind
edit win_platform.cpp
set splitbelow splitright
wincmd t
set winminheight=0
set winheight=1
set winminwidth=0
set winwidth=1
argglobal
setlocal keymap=
setlocal noarabic
setlocal autoindent
setlocal backupcopy=
setlocal balloonexpr=
setlocal nobinary
setlocal nobreakindent
setlocal breakindentopt=
setlocal bufhidden=
setlocal buflisted
setlocal buftype=
setlocal cindent
setlocal cinkeys=0{,0},0),0],:,0#,!^F,o,O,e
setlocal cinoptions=
setlocal cinwords=if,else,while,do,for,switch
setlocal colorcolumn=
setlocal comments=sO:*\ -,mO:*\ \ ,exO:*/,s1:/*,mb:*,ex:*/,://
setlocal commentstring=/*%s*/
setlocal complete=.,w,b,u,t,i
setlocal concealcursor=
setlocal conceallevel=0
setlocal completefunc=
setlocal completeslash=
setlocal nocopyindent
setlocal cryptmethod=
setlocal nocursorbind
setlocal nocursorcolumn
setlocal nocursorline
setlocal cursorlineopt=both
setlocal define=
setlocal dictionary=
setlocal nodiff
setlocal equalprg=
setlocal errorformat=
setlocal expandtab
if &filetype != 'cpp'
setlocal filetype=cpp
endif
setlocal fixendofline
setlocal foldcolumn=0
setlocal foldenable
setlocal foldexpr=0
setlocal foldignore=#
setlocal foldlevel=0
setlocal foldmarker={{{,}}}
setlocal foldmethod=manual
setlocal foldminlines=1
setlocal foldnestmax=20
setlocal foldtext=foldtext()
setlocal formatexpr=
setlocal formatoptions=croql
setlocal formatlistpat=^\\s*\\d\\+[\\]:.)}\\t\ ]\\s*
setlocal formatprg=
setlocal grepprg=
setlocal iminsert=0
setlocal imsearch=-1
setlocal include=
setlocal includeexpr=
setlocal indentexpr=
setlocal indentkeys=0{,0},0),0],:,0#,!^F,o,O,e
setlocal noinfercase
setlocal iskeyword=@,48-57,_,192-255
setlocal keywordprg=
set linebreak
setlocal linebreak
setlocal nolisp
setlocal lispwords=
setlocal nolist
setlocal makeencoding=
setlocal makeprg=
setlocal matchpairs=(:),{:},[:]
setlocal modeline
setlocal modifiable
setlocal nrformats=bin,octal,hex
set number
setlocal number
setlocal numberwidth=4
setlocal omnifunc=ccomplete#Complete
setlocal path=
setlocal nopreserveindent
setlocal nopreviewwindow
setlocal quoteescape=\\
setlocal noreadonly
setlocal norelativenumber
setlocal norightleft
setlocal rightleftcmd=search
setlocal noscrollbind
setlocal scrolloff=-1
setlocal shiftwidth=4
setlocal noshortname
setlocal showbreak=
setlocal sidescrolloff=-1
setlocal signcolumn=auto
setlocal smartindent
setlocal softtabstop=4
setlocal nospell
setlocal spellcapcheck=[.?!]\\_[\\])'\"\	\ ]\\+
setlocal spellfile=
setlocal spelllang=en
setlocal statusline=
setlocal suffixesadd=
setlocal swapfile
setlocal synmaxcol=3000
if &syntax != 'cpp'
setlocal syntax=cpp
endif
setlocal tabstop=4
setlocal tagcase=
setlocal tagfunc=
setlocal tags=
setlocal termwinkey=
setlocal termwinscroll=10000
setlocal termwinsize=
setlocal textwidth=0
setlocal thesaurus=
setlocal noundofile
setlocal undolevels=-123456
setlocal varsofttabstop=
setlocal vartabstop=
setlocal wincolor=
setlocal nowinfixheight
setlocal nowinfixwidth
setlocal wrap
setlocal wrapmargin=0
silent! normal! zE
let s:l = 1 - ((0 * winheight(0) + 25) / 51)
if s:l < 1 | let s:l = 1 | endif
exe s:l
normal! zt
1
normal! 0
tabnext
edit game.h
set splitbelow splitright
wincmd t
set winminheight=0
set winheight=1
set winminwidth=0
set winwidth=1
argglobal
setlocal keymap=
setlocal noarabic
setlocal autoindent
setlocal backupcopy=
setlocal balloonexpr=
setlocal nobinary
setlocal nobreakindent
setlocal breakindentopt=
setlocal bufhidden=
setlocal buflisted
setlocal buftype=
setlocal cindent
setlocal cinkeys=0{,0},0),0],:,0#,!^F,o,O,e
setlocal cinoptions=
setlocal cinwords=if,else,while,do,for,switch
setlocal colorcolumn=
setlocal comments=sO:*\ -,mO:*\ \ ,exO:*/,s1:/*,mb:*,ex:*/,://
setlocal commentstring=/*%s*/
setlocal complete=.,w,b,u,t,i
setlocal concealcursor=
setlocal conceallevel=0
setlocal completefunc=
setlocal completeslash=
setlocal nocopyindent
setlocal cryptmethod=
setlocal nocursorbind
setlocal nocursorcolumn
setlocal nocursorline
setlocal cursorlineopt=both
setlocal define=
setlocal dictionary=
setlocal nodiff
setlocal equalprg=
setlocal errorformat=
setlocal expandtab
if &filetype != 'cpp'
setlocal filetype=cpp
endif
setlocal fixendofline
setlocal foldcolumn=0
setlocal foldenable
setlocal foldexpr=0
setlocal foldignore=#
setlocal foldlevel=0
setlocal foldmarker={{{,}}}
setlocal foldmethod=manual
setlocal foldminlines=1
setlocal foldnestmax=20
setlocal foldtext=foldtext()
setlocal formatexpr=
setlocal formatoptions=croql
setlocal formatlistpat=^\\s*\\d\\+[\\]:.)}\\t\ ]\\s*
setlocal formatprg=
setlocal grepprg=
setlocal iminsert=0
setlocal imsearch=-1
setlocal include=
setlocal includeexpr=
setlocal indentexpr=
setlocal indentkeys=0{,0},0),0],:,0#,!^F,o,O,e
setlocal noinfercase
setlocal iskeyword=@,48-57,_,192-255
setlocal keywordprg=
set linebreak
setlocal linebreak
setlocal nolisp
setlocal lispwords=
setlocal nolist
setlocal makeencoding=
setlocal makeprg=
setlocal matchpairs=(:),{:},[:]
setlocal modeline
setlocal modifiable
setlocal nrformats=bin,octal,hex
set number
setlocal number
setlocal numberwidth=4
setlocal omnifunc=ccomplete#Complete
setlocal path=
setlocal nopreserveindent
setlocal nopreviewwindow
setlocal quoteescape=\\
setlocal noreadonly
setlocal norelativenumber
setlocal norightleft
setlocal rightleftcmd=search
setlocal noscrollbind
setlocal scrolloff=-1
setlocal shiftwidth=4
setlocal noshortname
setlocal showbreak=
setlocal sidescrolloff=-1
setlocal signcolumn=auto
setlocal smartindent
setlocal softtabstop=4
setlocal nospell
setlocal spellcapcheck=[.?!]\\_[\\])'\"\	\ ]\\+
setlocal spellfile=
setlocal spelllang=en
setlocal statusline=
setlocal suffixesadd=
setlocal swapfile
setlocal synmaxcol=3000
if &syntax != 'cpp'
setlocal syntax=cpp
endif
setlocal tabstop=4
setlocal tagcase=
setlocal tagfunc=
setlocal tags=
setlocal termwinkey=
setlocal termwinscroll=10000
setlocal termwinsize=
setlocal textwidth=0
setlocal thesaurus=
setlocal noundofile
setlocal undolevels=-123456
setlocal varsofttabstop=
setlocal vartabstop=
setlocal wincolor=
setlocal nowinfixheight
setlocal nowinfixwidth
setlocal wrap
setlocal wrapmargin=0
silent! normal! zE
let s:l = 14 - ((13 * winheight(0) + 25) / 50)
if s:l < 1 | let s:l = 1 | endif
exe s:l
normal! zt
14
normal! 0
tabnext 2
set stal=1
badd +1 win_platform.cpp
badd +14 game.h
if exists('s:wipebuf') && len(win_findbuf(s:wipebuf)) == 0
  silent exe 'bwipe ' . s:wipebuf
endif
unlet! s:wipebuf
set winheight=1 winwidth=20 shortmess=filnxtToOS
set winminheight=1 winminwidth=1
let s:sx = expand("<sfile>:p:r")."x.vim"
if file_readable(s:sx)
  exe "source " . fnameescape(s:sx)
endif
let &so = s:so_save | let &siso = s:siso_save
nohlsearch
doautoall SessionLoadPost
unlet SessionLoad
" vim: set ft=vim :
