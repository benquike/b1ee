all : b1ee


OBJDIR := obj
SRCDIR := src


OBJS := $(addprefix $(OBJDIR)/,\
   main.o log.o \
	socket.o listen_socket.o client_socket.o web_socket.o \
	controller.o lowerhci.o \
	linklayer.o llsm.o llsm_adv.o llsm_scan.o \
   phylayer.o )


DEPENDS := $(OBJS:.o=.d)


clean :
	@echo "Removing object files"
	@rm obj/*


b1ee : $(OBJS) $(DEPENDS)
	@echo "Linking $@"
	@c++ -o $@ -pthread $(OBJS)
	@echo "-------------------------------------------------------------------------------"


$(OBJDIR)/%.o : $(SRCDIR)/%.cpp makefile
	@echo "Compiling $<"
	@c++ -g -pthread -Werror -c $< -o $@


$(OBJDIR)/%.d : $(SRCDIR)/%.cpp makefile
	@c++ -MM -MT $(OBJDIR)/$*.o -o $(OBJDIR)/$*.d $<


-include $(DEPENDS)
