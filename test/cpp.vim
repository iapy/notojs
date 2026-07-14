if exists('b:current_syntax')
  let old_syntax = b:current_syntax
  unlet b:current_syntax
endif

syn include @css syntax/css.vim
unlet b:current_syntax

syn include @javascript syntax/javascript.vim
unlet b:current_syntax

syn include @json syntax/json.vim
unlet b:current_syntax

syn match EmbedControl /R"CSS(/
syn match EmbedControl /)CSS"/

syn match EmbedControl /R"JS(/
syn match EmbedControl /)JS"/

syn match EmbedControl /R"JSON(/
syn match EmbedControl /)JSON"/

syn region EmbedCSS start=/R"CSS(/rs=s,hs=s,ms=s end=/)CSS"/re=s-1,me=s-1 contains=EmbedControl,@css keepend matchgroup=Embed
syn region EmbedJSON start=/R"JSON(/rs=s,hs=s,ms=s end=/)JSON"/re=s-1,me=s-1 contains=EmbedControl,@json keepend matchgroup=Embed
syn region EmbedJavaScript start=/R"JS(/rs=s,hs=s,ms=s end=/)JS"/re=s-1,me=s-1 contains=EmbedControl,@javascript keepend matchgroup=Embed

hi def link EmbedControl cType

syn match BoostTest /BOOST_TEST/
syn match BoostTest /BOOST_AUTO_TEST_CASE/
syn match BoostExtra /BOOST_FORCEINLINE/

hi def link BoostTest cOperator
hi def link BoostExtra cppModifier

syn sync clear
if exists('old_syntax')
  let b:current_syntax = old_syntax
endif
