macro_rules! try_enum_from_repr {
    {
        from = $from: ty,
        to = $to: ty,
        error = $error: ty,
        variants = {$($variant:expr),+ $(,)?},
        fallback = $fallback:expr $(,)?
    } => {
        impl core::convert::TryFrom<$from> for $to {
            type Error = $error;

            fn try_from(value: $from) -> Result<Self, Self::Error> {
                match value {
                    $(v if v == $variant as $from => Ok($variant),)*
                    _ => Err($fallback),
                }
            }

        }

    };
}
pub(super) use try_enum_from_repr;

macro_rules! define_mailbox_message {
    {
         // define_response! {
         //    tag1_name => {
         //      response_name1: response_type1
         //      response_name2: response_type2
         //    },
         //    tag2_name => {
         //      response_name1: response_type1
         //    }
         // }
        $name:ident,
        $($tag:ident => {
            $($element:ident: $ty:ty),+ $(,)?
        }),+ $(,)?,
    } => {
        paste::paste! {
            #[allow(dead_code)]
            #[repr(C, align(16))]
            struct $name {
                /// Size of the message buffer, in bytes.
                buffer_size: u32,
                /// Request/response code.
                code: u32,
                $(
                    /// Tag identifier. This should be the tag identifier for the request.
                    [< $tag _identifier >]: TagIdentifier,
                    /// Size of the response value buffer, in bytes.
                    [< $tag _buffer_size >]: u32,
                    /// Response code. This should be 0 for requests.
                    [< $tag _code >]: u32,
                    $(
                        /// Response value.
                        $element: $ty,
                    )*
                )*
                /// End tag identifier. This should be `TagIdentifier::End`.
                end_tag: TagIdentifier,
            }
        }
    };
}
pub(super) use define_mailbox_message;
