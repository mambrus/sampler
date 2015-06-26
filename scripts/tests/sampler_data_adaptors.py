def kwargs_get(d, name, value, keep=False):
    if d.has_key(name):
        value = d[name]
        if not keep:
            del d[name]
    return d, value

#
# Adaptor is an object which represents a connection between a data
# item and input/ouput/ui, keeping values updated in both directions
#
class Adaptor(object):
    def __init__(self, name, ui_name=None, ui_controller=None, ui_builder=None, ui_changed=None, ui_sensitivity=None, v_empty = None):
        self.name = name
        self.v_empty = v_empty
        self.ui = bool(ui_controller)
        if self.ui:
            self.ui_controller = ui_controller
            self.ui_builder = ui_builder
            self.ui_name = ui_name
            self.ui_widget = ui_builder.get_object(ui_name) if ui_name else None
            self.ui_changed = ui_changed
            self.ui_sensitivity = ui_sensitivity
    def ui_set_model(self, model, set_sensitivity=True):
        if self.ui:
            self.ui_set_model_impl(model)
            if set_sensitivity:
                self.ui_set_sensitivity(model)
    def ui_set_model_impl(self, model):
        pass # to be implemented by subclass
    def ui_on_changed(self, widget):
        #print "yo: %s() widget=%r text=%r" % (self.ui_callback_name, widget, widget.get_text())
        self.ui_changed()
    def ui_modify_model(self, model):
        pass # to be implemented by subclass
    def ui_set_sensitivity(self, model):
        if self.ui_sensitivity:
            self.ui_widget.set_sensitive(self.ui_sensitivity(model))
    def load(self, src, model):
        model[self.name] = src[self.name] if src.has_key(self.name) else self.v_empty
    def save(self, model, dst):
        if self.v_empty != model[self.name]:
            dst[self.name] = model[self.name]

class StringAdaptor(Adaptor):
    def __init__(self, name, **kwargs):
        kwargs, self.type   = kwargs_get(kwargs, 'type',   str)
        kwargs, self.s_none = kwargs_get(kwargs, 's_none', '')
        super(StringAdaptor, self).__init__(name, **kwargs)
        if self.ui:
            self.ui_callback_name = self.ui_get_callback_name()
            setattr(self.ui_controller, self.ui_callback_name, self.ui_on_changed)
    def ui_get_callback_name(self):
        return "on_%s_changed" % self.ui_name
    def ui_set_model_impl(self, model):
        v = model[self.name]
        s = str(v) if v != None else self.s_none
        self.ui_widget.set_text(s)
    def ui_modify_model(self, model):
        if self.ui:
            s = self.ui_widget.get_text().strip()
            try:
                if s == '':
                    v = self.v_empty
                else:
                    v = self.type(s)
                    if self.type == float:
                        i = int(v)
                        if v == i:
                            v = i # use integer representation instead of float if possible
                model[self.name] = v
            except Exception, e:
                text = "Invalid value for field %r of StringAdaptor(%r): %r" % (self.name, self.type.__name__, s)
                raise Exception(text)

class BooleanAdaptor(StringAdaptor):
    def __init__(self, *args, **kwargs):
        if not kwargs.has_key('type'): kwargs['type'] = bool
        if not kwargs.has_key('v_empty'): kwargs['v_empty'] = False
        super(BooleanAdaptor, self).__init__(*args, **kwargs)
    def ui_get_callback_name(self):
        return "on_%s_toggled" % self.ui_name
    def ui_set_model_impl(self, model):
        v = model[self.name]
        self.ui_widget.set_active(v)
    def ui_modify_model(self, model):
        if self.ui:
            v = self.ui_widget.get_active()
            model[self.name] = v

class StructAdaptor(Adaptor):
    def __init__(self, children_adaptors, **kwargs):
        kwargs, self.ui_placeholder_name   = kwargs_get(kwargs, 'ui_placeholder_name', None)
        super(StructAdaptor, self).__init__(None, **kwargs)
        self.children_adaptors = children_adaptors
        if self.ui:
            self.ui_placeholder = self.ui_builder.get_object(self.ui_placeholder_name) if self.ui_placeholder_name else None
    def ui_set_sensitivity(self, model):
        for a in self.children_adaptors:
            if a.ui:
                a.ui_set_sensitivity(model)
    def ui_modify_model(self, model):
        if self.ui:
            for a in self.children_adaptors:
                a.ui_modify_model(model)
    def ui_set_model_impl(self, model):
        self.setup_placeholder()
        for a in self.children_adaptors:
            a.ui_set_model(model, set_sensitivity=False)
    def setup_placeholder(self):
        #
        # Inserts "self.ui_widget" in "self.ui_placeholder".  This
        # allows for reusing this placeholder between different
        # StructAdaptors
        #
        old_children = self.ui_placeholder.get_children()
        old_child = old_children[0] if len(old_children) > 0 else None
        if old_child != self.ui_widget:
            if old_child:
                old_child_parent = old_child.get_parent()
                if old_child_parent:
                    old_child_parent.remove(old_child)
            if self.ui_widget:
                new_child_parent = self.ui_widget.get_parent()
                if new_child_parent:
                    new_child_parent.remove(self.ui_widget)
                self.ui_placeholder.add(self.ui_widget)
    def load(self, src, model):
        for a in self.children_adaptors:
            a.load(src, model)
    def save(self, model, dst):
        for a in self.children_adaptors:
            a.save(model, dst)

