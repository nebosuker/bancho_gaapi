(function($){$(document).ready(function(){
  if ($('table.bancho_gaapi_statistics')) {

    // thに特定のクラス名がある列だけcolorScaleを実行
    $('table.bancho_gaapi_statistics tr:first-child th').each(function(k,v){
      if ($(v).hasClass('bancho_gaapi_statistics_data')) {
        // :nth-childの指定は1ベース
        $('table.bancho_gaapi_statistics tr > :nth-child(' + (k+1) + ')').tableColorScale({
          'type': 'sigma',
          'css': {
            'avgTo1sigma' : {},
            'avgTo-1sigma' : {}
          }
        });

      }
    });

  }
});})(jQuery);
