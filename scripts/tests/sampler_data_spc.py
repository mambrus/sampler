import pprint
import re

from sampler_data_adaptors import *

#
# ui_controller is expected to implement the following interface
#
# ui_controller.is_signal_expectable()
# ui_controller.is_signal_exportable()
# ui_controller.is_signal_visible()
# ui_controller.mark_changed_common()
# ui_controller.mark_changed_signal()
# ui_controller.mark_changed_signal_and_update_sensitivity()
# ui_controller.mark_changed_signal_title()

SPC_FORMAT_VERSION = 2

class LambdaAdaptor(StringAdaptor):
    def __init__(self, name, **kwargs):
        if not kwargs.has_key('v_empty'): kwargs['v_empty'] = 'lambda x: x'
        if not kwargs.has_key('s_none'): kwargs['s_none'] = 'lambda x: x'
        super(LambdaAdaptor, self).__init__(name, **kwargs)
    def ui_set_model_impl(self, model):
        v = model[self.name]
        s = self.strip_lambda_prefix(str(v) if v != None else self.s_none)
        self.ui_widget.set_text(s)
    def ui_modify_model(self, model):
        if self.ui:
            s = self.strip_lambda_prefix(self.ui_widget.get_text().strip())
            if s == '':
                v = self.v_empty
            else:
                v = "lambda x: " + s
            model[self.name] = v
    def strip_lambda_prefix(self, s):
        lambda_prefix = "lambda x:"
        if s.startswith(lambda_prefix):
            s = s[len(lambda_prefix):].lstrip()
        return s

#
# ExportableBooleanAdaptor exists only for backwards compatibility
# with .spc files where "exportable" is missing for some signals. All
# .spc files created after introduction of the
# ExportableBooleanAdaptor (should be soon after 2013-09-13) can use
# BooleanAdapter without a default value. This means that
# ExportableBooleanAdaptor can be removed in the future.
#
class ExportableBooleanAdaptor(BooleanAdaptor):
    def __init__(self, *args, **kwargs):
        super(ExportableBooleanAdaptor, self).__init__(*args, **kwargs)
    def load(self, src, model):
        not_toplevel = src.has_key('subsignal') and src['subsignal']
        default_exportable = not bool(not_toplevel)
        model[self.name] = src[self.name] if src.has_key(self.name) else default_exportable
    def save(self, model, dst):
        dst[self.name] = model[self.name]

def build_adaptor_signal(ui_controller, ui_builder):
    c = ui_controller
    kwargs = {'ui_controller': c,
              'ui_builder': ui_builder,
              'ui_changed': (c.mark_changed_signal if c else None)}
    return StructAdaptor(
        [StringAdaptor("id",
                       **dict(kwargs,
                              ui_name="signal_id",
                              ui_changed=(c.mark_changed_signal_title if c else None))),
         StringAdaptor("time",
                       **dict(kwargs,
                              ui_name="signal_time",
                              s_none='Time-now',
                              v_empty='Time-now')),
         LambdaAdaptor("expr",
                       **dict(kwargs,
                              ui_name="signal_expr")),
         ExportableBooleanAdaptor("exportable",
                        **dict(kwargs,
                               ui_name="signal_exportable",
                               ui_changed=(c.mark_changed_signal_and_update_sensitivity if c else None))),
         StringAdaptor("fname",
                       **dict(kwargs,
                              ui_name="signal_fname",
                              ui_sensitivity=(c.is_signal_exportable if c else None))),
         StringAdaptor("fdata",
                       **dict(kwargs,
                              ui_name="signal_fdata",
                              ui_sensitivity=(c.is_signal_exportable if c else None))),
         StringAdaptor("fops_mask",
                       **dict(kwargs,
                              ui_name="signal_fops_mask",
                              s_none='1',
                              v_empty='1',
                              ui_sensitivity=(c.is_signal_exportable if c else None))),
         StringAdaptor("rgx_line",
                       **dict(kwargs,
                              ui_name="signal_rgx_line",
                              ui_sensitivity=(c.is_signal_exportable if c else None))),
         StringAdaptor("rgx_line_index",
                       **dict(kwargs,
                              ui_name="signal_rgx_line_index",
                              type=int,
                              ui_sensitivity=(c.is_signal_exportable if c else None))),
         StringAdaptor("rgx_sig",
                       **dict(kwargs,
                              ui_name="signal_rgx_sig",
                              ui_sensitivity=(c.is_signal_exportable if c else None))),
         StringAdaptor("rgx_sig_index",
                       **dict(kwargs,
                              ui_name="signal_rgx_sig_index",
                              ui_sensitivity=(c.is_signal_exportable if c else None))),
         StringAdaptor("nuce",
                       **dict(kwargs,
                              ui_name="signal_nuce",
                              type=int,
                              s_none='0',
                              v_empty=0,
                              ui_sensitivity=(c.is_signal_exportable if c else None))),
         BooleanAdaptor("expectable",
                        **dict(kwargs,
                               ui_name="signal_expectable",
                               v_empty=True,
                               ui_changed=(c.mark_changed_signal_and_update_sensitivity if c else None))),
         StringAdaptor("nosig",
                       **dict(kwargs,
                              ui_name="signal_nosig",
                              ui_sensitivity=(c.is_signal_expectable if c else None))),
         BooleanAdaptor("visible",
                        **dict(kwargs,
                               ui_name="signal_visible",
                               v_empty=True,
                               ui_changed=(c.mark_changed_signal_and_update_sensitivity if c else None))),
         StringAdaptor("name",
                       **dict(kwargs,
                              ui_name="signal_name",
                              ui_sensitivity=(c.is_signal_visible if c else None))),
         LambdaAdaptor("scale",
                       **dict(kwargs,
                              ui_name="signal_scale",
                              ui_sensitivity=(c.is_signal_visible if c else None))),
         StringAdaptor("y",
                       **dict(kwargs,
                              ui_name="signal_y",
                              type=int,
                              s_none='1',
                              v_empty=1,
                              ui_sensitivity=(c.is_signal_visible if c else None))),
         StringAdaptor("style",
                       **dict(kwargs,
                              ui_name="signal_style",
                              ui_sensitivity=(c.is_signal_visible if c else None))),
         Adaptor("vars", v_empty={}), # this is for compatibility only
         Adaptor("subsignal", v_empty=0),
         ],
        **dict(kwargs, ui_name='editor_signal', ui_placeholder_name='item_editor'))

def build_adaptor_common(ui_controller, ui_builder):
    c = ui_controller
    kwargs = {'ui_controller': c,
              'ui_builder': ui_builder,
              'ui_changed': (c.mark_changed_common if c else None)}
    return StructAdaptor(
        [StringAdaptor("sigs_file",      **dict(kwargs, ui_name="common_sigs_file")),
         BooleanAdaptor("host",          **dict(kwargs, ui_name="common_host")),
         StringAdaptor("period",         **dict(kwargs, ui_name="common_period",   type=float, s_none='0', v_empty=0)),
         StringAdaptor("extra_feedgnuplot", **dict(kwargs, ui_name="common_extra_feedgnuplot")),
         BooleanAdaptor("time_columns",  **dict(kwargs, ui_name="common_time_columns", v_empty=True)),
         StringAdaptor("window",         **dict(kwargs, ui_name="common_window",   type=float, s_none='1.0', v_empty=1.0)),
         StringAdaptor("y_shift",        **dict(kwargs, ui_name="common_y_shift",  type=float, s_none='0', v_empty=0)),
         StringAdaptor("y2_shift",       **dict(kwargs, ui_name="common_y2_shift", type=float, s_none='0', v_empty=0)),
         ],
        **dict(kwargs, ui_name='editor_common', ui_placeholder_name='item_editor'))

spc_re = re.compile("^(?={)", re.M)

def module_strip(s):
    lines = s.split("\n")
    while len(lines) > 0 and lines[-1].strip() in ["#", ""]:
        del lines[-1]
    return ("\n".join(lines))+"\n"

def load(adaptor_common, adaptor_signal, filename):
    #
    # Loads a new file without error handling. Callers
    # responsibility is to handle errors and call
    # "self.set_model()" on success.
    #
    src_module, src_structure = None, file(filename).read()
    m = spc_re.search(src_structure)
    if m:
        src_module, src_structure = src_structure[:m.start()], src_structure[m.start():]
        src_module = module_strip(src_module)
    data_common_raw = eval(src_structure)
    if data_common_raw.has_key('version'):
        if data_common_raw['version'] > SPC_FORMAT_VERSION:
            text = 'failed to load "%s" file which uses new format version %d but the sampler software supports only up to %d' % (filename, data_common_raw['version'], SPC_FORMAT_VERSION)
            raise Exception(text)
        del data_common_raw['version']
    data_common = {'signals': []}
    adaptor_common.load(data_common_raw, data_common)
    for data_signal_raw in data_common_raw['signals']:
        data_signal = {}
        adaptor_signal.load(data_signal_raw, data_signal)
        data_common['signals'].append(data_signal)
    return data_common, src_module

def save(adaptor_common, adaptor_signal, filename, model, src_module):
    data_common_raw = {'signals': []}
    adaptor_common.save(model, data_common_raw)
    for data_signal in model['signals']:
        data_signal_raw = {}
        adaptor_signal.save(data_signal, data_signal_raw)
        data_common_raw['signals'].append(data_signal_raw)

    data_common_raw['version'] = SPC_FORMAT_VERSION
    with file(filename, 'w') as stream:
        if src_module:
            src_module = module_strip(src_module)
            print >>stream, src_module
        else:
            print >>stream, "# -*- python -*-"
            print >>stream, ""
        pp = pprint.PrettyPrinter(stream=stream)
        pp.pprint(data_common_raw)

def format_str(value, empty=''):
    #
    # Helper function used in export() for converting string
    # values
    #
    return value if value else empty

def format_int(value, empty=''):
    #
    # Helper function used in export() for converting int
    # values
    #
    return value if value else empty

def is_signal_exportable(model):
    return model['exportable']

def export(adaptor_common, adaptor_signal, f, model):
    #
    # Exports model to "ini" file for "sampler"
    #
    if type(f) == str:
        f = file(f, "w")
    with f:
        for signal in model['signals']:
            if is_signal_exportable(signal):
                fields = (format_str(signal['id']),
                          format_str(signal['fname']),
                          format_str(signal['fdata']),
                          format_str(signal['fops_mask'], empty='1'),
                          format_str(signal['rgx_line']),
                          format_int(signal['rgx_line_index']),
                          format_str(signal['rgx_sig']),
                          format_str(signal['rgx_sig_index']),
                          format_int(signal['nuce']))
                print >>f, "%s;%s;%s;%s;%s;%s;%s;%s;%s" % fields
