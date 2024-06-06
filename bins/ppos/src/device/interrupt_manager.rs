use library::sync::mutex::Mutex;

use super::interrupt_controller::InterruptNumber;

static CUR_INTERRUPT_MANAGER: Mutex<
    &'static (dyn InterruptManager<InterruptNumberType = InterruptNumber> + Sync),
> = Mutex::new(&NULL_INTERRUPT_MANAGER);
static NULL_INTERRUPT_MANAGER: NullInterruptManager = NullInterruptManager {};

struct NullInterruptManager {}

impl InterruptManager for NullInterruptManager {
    type InterruptNumberType = InterruptNumber;

    fn register_handler(
        &self,
        _: InterruptHandlerDescriptor<Self::InterruptNumberType>,
    ) -> Result<(), &'static str> {
        unimplemented!()
    }

    fn enable(&self, _: &Self::InterruptNumberType) {
        unimplemented!()
    }

    fn handle_pending_interrupt(&self) {
        unimplemented!()
    }
}

pub fn register_interrupt_manager(
    new_manager: &'static (dyn InterruptManager<InterruptNumberType = InterruptNumber> + Sync),
) {
    let mut cur_interrupt_manager = CUR_INTERRUPT_MANAGER.lock().unwrap();
    *cur_interrupt_manager = new_manager;
}

pub fn interrupt_manager(
) -> &'static (dyn InterruptManager<InterruptNumberType = InterruptNumber> + Sync) {
    *CUR_INTERRUPT_MANAGER.lock().unwrap()
}

pub trait InterruptHandler {
    /// Called when the corresponding interrupt is asserted.
    fn handle(&self) -> Result<(), &'static str>;
}

pub trait InterruptPrehook {
    /// Called when the coresponding interrupt handler is added to queue
    fn prehook(&self) -> Result<(), &'static str>;
}

#[derive(Copy, Clone)]
pub struct InterruptHandlerDescriptor<T>
where
    T: Copy,
{
    /// The interrupt number.
    number: T,

    /// Descriptive name.
    name: &'static str,

    /// Reference to prehook trait object
    prehook: Option<&'static (dyn InterruptPrehook + Sync + Send)>,

    /// Reference to handler trait object.
    handler: &'static (dyn InterruptHandler + Sync + Send),

    /// Priority of the interrupt. Lower value means higher priority.
    priority: usize,
}

impl<T> InterruptHandlerDescriptor<T>
where
    T: Copy,
{
    /// Create an instance.
    pub const fn new(
        number: T,
        name: &'static str,
        prehook: Option<&'static (dyn InterruptPrehook + Sync + Send)>,
        handler: &'static (dyn InterruptHandler + Sync + Send),
        priority: usize,
    ) -> Self {
        Self {
            number,
            name,
            prehook,
            handler,
            priority,
        }
    }

    /// Return the number.
    #[inline(always)]
    pub const fn number(&self) -> T {
        self.number
    }

    /// Return the name.
    #[inline(always)]
    pub const fn name(&self) -> &'static str {
        self.name
    }

    /// Return the prehook
    #[inline(always)]
    pub const fn prehook(&self) -> Option<&'static (dyn InterruptPrehook + Sync + Send)> {
        self.prehook
    }

    /// Return the handler.
    #[inline(always)]
    pub const fn handler(&self) -> &'static (dyn InterruptHandler + Sync + Send) {
        self.handler
    }

    /// Return the priority.
    #[inline(always)]
    pub const fn priority(&self) -> usize {
        self.priority
    }
}

pub trait InterruptManager {
    type InterruptNumberType: Copy;

    fn register_handler(
        &self,
        interrupt_handler_descriptor: InterruptHandlerDescriptor<Self::InterruptNumberType>,
    ) -> Result<(), &'static str>;

    fn enable(&self, interrupt_number: &Self::InterruptNumberType);

    fn handle_pending_interrupt(&self);
}
