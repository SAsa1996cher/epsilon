app_shared_test_src = $(addprefix apps/shared/,\
  continuous_function.cpp \
  continuous_function_cache.cpp \
  continuous_function_properties.cpp \
  continuous_function_store.cpp \
  curve_selection_controller.cpp \
  curve_view_cursor.cpp \
  curve_view_range.cpp \
  dots.cpp \
  double_pair_store.cpp \
  expression_display_permissions.cpp \
  expression_model.cpp \
  expression_model_handle.cpp \
  expression_model_store.cpp \
  function.cpp \
  global_context.cpp \
  inference.cpp \
  interactive_curve_view_range.cpp \
  interactive_curve_view_range_delegate.cpp \
  interval.cpp \
  linear_regression_store.cpp \
  memoized_curve_view_range.cpp \
  packed_range_1D.cpp \
  plot_view.cpp \
  plot_view_axes.cpp \
  plot_view_banners.cpp \
  plot_view_cursors.cpp \
  plot_view_plots.cpp \
  sequence.cpp \
  sequence_cache_context.cpp \
  sequence_context.cpp \
  sequence_store.cpp \
  toolbox_helpers.cpp \
  zoom_and_pan_curve_view_controller.cpp \
  zoom_curve_view_controller.cpp \
)

app_shared_src = $(addprefix apps/shared/,\
  banner_view.cpp \
  buffer_function_title_cell.cpp \
  button_with_separator.cpp \
  cell_with_separator.cpp \
  color_cell.cpp \
  color_names.cpp \
  color_parameter_controller.cpp \
  column_helper.cpp \
  column_parameter_controller.cpp \
  double_pair_table_controller.cpp \
  editable_cell_selectable_table_view.cpp \
  editable_cell_table_view_controller.cpp \
  explicit_float_parameter_controller.cpp \
  expression_display_permissions.cpp \
  expression_field_delegate_app.cpp \
  expression_function_title_cell.cpp \
  expression_model_list_controller.cpp \
  float_parameter_controller.cpp \
  formula_template_menu_controller.cpp \
  function_app.cpp \
  function_banner_delegate.cpp \
  function_go_to_parameter_controller.cpp \
  function_graph_controller.cpp \
  function_graph_view.cpp \
  function_list_controller.cpp \
  function_title_cell.cpp \
  function_zoom_and_pan_curve_view_controller.cpp \
  go_to_parameter_controller.cpp \
  input_event_handler_delegate_app.cpp \
  interactive_curve_view_controller.cpp \
  interval.cpp \
  interval_parameter_controller.cpp \
  layout_field_delegate.cpp \
  list_parameter_controller.cpp \
  localization_controller.cpp \
  memoized_cursor_view.cpp \
  menu_controller.cpp \
  message_view.cpp \
  ok_view.cpp \
  parameter_text_field_delegate.cpp \
  prefaced_table_view.cpp \
  prefaced_twice_table_view.cpp \
  range_parameter_controller.cpp \
  round_cursor_view.cpp \
  scrollable_multiple_expressions_view.cpp \
  scrollable_two_expressions_cell.cpp \
  separable.cpp \
  separator_even_odd_buffer_text_cell.cpp \
  sequence.cpp \
  sequence_cache_context.cpp \
  sequence_context.cpp \
  sequence_store.cpp \
  shared_app.cpp \
  simple_interactive_curve_view_controller.cpp \
  single_interactive_curve_view_range_controller.cpp \
  single_range_controller.cpp \
  store_cell.cpp \
  store_controller.cpp \
  store_parameter_controller.cpp \
  store_title_cell.cpp \
  store_menu_controller.cpp \
  sum_graph_controller.cpp \
  tab_table_controller.cpp \
  text_field_delegate.cpp \
  text_field_delegate_app.cpp \
  text_field_with_extension.cpp \
  values_controller.cpp \
  values_parameter_controller.cpp \
  vertical_cursor_view.cpp \
  xy_banner_view.cpp \
)

app_shared_src += $(app_shared_test_src)
apps_src += $(app_shared_src)

i18n_files += $(call i18n_with_universal_for,shared/colors)

tests_src += $(addprefix apps/shared/test/,\
  function_alignement.cpp \
  interval.cpp \
)
