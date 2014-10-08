<?php
/**
 * @file views-view-bancho-gaapi-ranking.tpl.php
 * Default simple view template to display a list of rows.
 *
 * - $title : The title of this group of rows.  May be empty.
 * - $options['type'] will either be ul or ol.
 * @ingroup views_templates
 *
 * views-view-list.tpl.phpと同じ
 */
?>
<div class="item-list">
  <?php if (!empty($title)) : ?>
    <h3><?php print $title; ?></h3>
  <?php endif; ?>
  <<?php print $options['type']; ?><?php if (isset($options['ul_classname'])) : ?> class="<?php echo $options['ul_classname']; ?>"<?php endif; ?>>
    <?php foreach ($rows as $id => $row): ?>
      <li<?php if (!empty($li_classes)) : ?> class="<?php print $li_classes[$id]; ?>"<?php endif; ?><?php if (!empty($li_values)) : ?> value="<?php echo $li_values[$id] ?>"<?php endif; ?>><?php print $row; ?></li>
    <?php endforeach; ?>
  </<?php print $options['type']; ?>>
</div>