from types import FunctionType

class MetaSelfWrapper(type):
    def __new__(cls, clsname, bases, dict):
        # collects first, for use in closure's context
        functions = {}
        for name, func in dict.iteritems():
            if isinstance(func, (FunctionType,property)):
                #print 'collecting', name, func
                functions[name] = func

        def makeWrapper(f):
            def wrapperFunc(self, *a, **kw):
                f.func_globals['self'] = self
                return f(*a, **kw)
            return wrapperFunc

        for name, f in functions.iteritems():
            if isinstance(f, property):
                # methods referenced in properties also have to 
                # be wrapped
                attrs = {'fget' : f.fget, 'fset' : f.fset,
                       'fdel' : f.fdel, 'doc' : f.__doc__}
                for attrname, value in attrs.items():
                    if value:
                        attrs[attrname] = makeWrapper(value)
                    else:
                        del attrs[attrname]
                p = property(**attrs)
                dict[name] = p
            else:
                f.func_globals.update(functions)
                dict[name] = makeWrapper(f)

        return type.__new__(cls, clsname, bases, dict)
    # __new__

class SelfWrapper(object):
    __metaclass__ = MetaSelfWrapper

