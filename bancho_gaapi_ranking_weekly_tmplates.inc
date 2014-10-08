<?php
/**
 * @file
 * Web担の週間などランキング記事用のHTMLテンプレート類
 */


function bancho_gaapi_ranking_weekly_tmplates(){

  return array(
    'teaser_icon_img_classes' => array('lazyload'),

    'teaser_icon_link_classes' => array('teaser_icon'),
    
    'created' => array(
      'prefix' => '<span class="created">',
      'suffix' => '</span>'
    ),
    
    'teaser_text' => array(
      'prefix' => '<div class="teaser">',
      'suffix' => '</div>'
    ),

    'sbm' => array(
      'prefix' => '<div class="node-property-line-1">',
      'suffix' => '</div>'
    ),

    'title' => array(
      'prefix' => '<p class="ranking_title">',
      'suffix' => '</p>'
    ),

    'title_corner_name' => array(
      'prefix' => '<span class="node-property-rensai-term">',
      'suffix' => '</span>'
    ),

    'ranking_body_1' => '<li id="ranking_sessions_%02d" class="clearfix">%s%s<div class="node-property"><div class="node-property-line1">%s%s</div></div>%s</li>%s',

    'ranking_body_2' => '<li value="%d" class="clearfix">%s%s<div class="node-property"><div class="node-property-line1">%s%s</div></div>%s</li>%s',

    'body_html' => '
<div<%HEADER_ICON_CLEARFIX%>>
<div class="image-box-right" style="width: 120px;">
<img src="/files/images/session_ranking_icon120.gif" alt="アクセス数上位ランキング" width="120" height="120" />
</div>

<p><%RANKING_PERIOD%>にアクセス数の多かった記事のランキングを発表！ 見逃している人気記事はありませんか？</p>
</div>

<ol id="ranking_sessions">

<%RANKING_BODY1%>
</ol>


<ol class="strong_marker">

<%RANKING_BODY2%>

</ol>

<p>※期間内のPV数によるランキングです。一部のまとめ記事や殿堂入り記事はランキング集計から除外されています。</p>

',

    'teaser_html' => '<p><%RANKING_PERIOD%>にアクセス数の多かった記事のランキングを発表！ 見逃している人気記事はありませんか？ 他のユーザーはどんな記事を読んでいるのか？先週1週間で最も人気の高かった記事を要チェック！</p>',



  );
}
