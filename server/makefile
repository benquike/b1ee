all : b1ee


OBJDIR := obj
SRCDIR := src


OBJS := $(addprefix $(OBJDIR)/,\
   main.o log.o socket.o controller.o lowerhci.o linklayer.o \
   llsm.o llsm_adv.o llsm_scan.o \
   phylayer.o )


DEPENDS := $(OBJS:.o=.d)


clean :
	@echo "Removing object files"
	@rm obj/*


b1ee : $(OBJS) $(DEPENDS)
	@echo "Linking $@"
	@c++ -o $@ $(OBJS)
	@echo "-------------------------------------------------------------------------------"


$(OBJDIR)/%.o : $(SRCDIR)/%.cpp makefile
	@echo "Compiling $<"
	@c++ -g -Werror -c $< -o $@


$(OBJDIR)/%.d : $(SRCDIR)/%.cpp makefile
	@c++ -MM -MT $(OBJDIR)/$*.o -o $(OBJDIR)/$*.d $<


-include $(DEPENDS)
