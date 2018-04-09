// Copyright 2016 Google Inc.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.!

#include "trainer_interface.h"
#include "testharness.h"
#include "util.h"

namespace sentencepiece {

// Space symbol
#define WS "\xe2\x96\x81"

TEST(TrainerInterfaceTest, IsValidSentencePieceTest) {
  TrainerSpec trainer_spec;
  NormalizerSpec normalizer_spec;

  auto IsValid = [&trainer_spec, &normalizer_spec](const std::string &str) {
    TrainerInterface trainer(trainer_spec, normalizer_spec);
    const string_util::UnicodeText text = string_util::UTF8ToUnicodeText(str);
    return trainer.IsValidSentencePiece(text);
  };

  // Default trainer spec.
  EXPECT_FALSE(IsValid(""));
  EXPECT_FALSE(IsValid("12345678912345678"));  // too long
  EXPECT_TRUE(IsValid("a"));
  EXPECT_TRUE(IsValid(WS));
  EXPECT_TRUE(IsValid(WS "a"));
  EXPECT_FALSE(IsValid("a" WS));
  EXPECT_FALSE(IsValid(WS "a" WS));
  EXPECT_FALSE(IsValid("a" WS "b"));
  EXPECT_FALSE(IsValid("a" WS "b" WS));
  EXPECT_TRUE(IsValid("あいう"));
  EXPECT_TRUE(IsValid("グーグル"));  // "ー" is a part of Katakana
  EXPECT_TRUE(IsValid("食べる"));
  EXPECT_FALSE(IsValid("漢字ABC"));  // mixed CJK scripts
  EXPECT_FALSE(IsValid("F1"));
  EXPECT_TRUE(IsValid("$10"));  // $ and 1 are both "common" script.
  EXPECT_FALSE(IsValid("$ABC"));
  EXPECT_FALSE(IsValid("ab\tbc"));  // "\t" is UPP boundary.

  trainer_spec.set_split_by_whitespace(false);
  EXPECT_TRUE(IsValid(WS));
  EXPECT_TRUE(IsValid(WS "a"));
  EXPECT_FALSE(IsValid("a" WS));
  EXPECT_FALSE(IsValid(WS "a" WS));
  EXPECT_TRUE(IsValid("a" WS "b"));  // "a b" is a valid piece.
  EXPECT_TRUE(IsValid(WS "a" WS "b"));
  EXPECT_TRUE(IsValid(WS "a" WS "b" WS "c"));
  EXPECT_FALSE(IsValid("a" WS "b" WS));

  trainer_spec.set_split_by_unicode_script(false);
  EXPECT_TRUE(IsValid("あいう"));
  EXPECT_TRUE(IsValid("グーグル"));
  EXPECT_TRUE(IsValid("食べる"));
  EXPECT_TRUE(IsValid("漢字ABC"));
  EXPECT_TRUE(IsValid("F1"));
  EXPECT_TRUE(IsValid("$10"));
  EXPECT_TRUE(IsValid("$ABC"));

  trainer_spec.set_max_sentencepiece_length(4);
  EXPECT_TRUE(IsValid("1234"));
  EXPECT_FALSE(IsValid("12345"));
}

TEST(TrainerInterfaceTest, OverrideSpecialPieces) {
  TrainerSpec trainer_spec;
  NormalizerSpec normalizer_spec;

  // Check default values.
  EXPECT_EQ(0, trainer_spec.unk_id());
  EXPECT_EQ(1, trainer_spec.bos_id());
  EXPECT_EQ(2, trainer_spec.eos_id());
  EXPECT_EQ(-1, trainer_spec.pad_id());

  {
    trainer_spec.set_unk_id(0);
    trainer_spec.set_bos_id(1);
    trainer_spec.set_eos_id(2);
    trainer_spec.set_pad_id(3);

    TrainerInterface trainer(trainer_spec, normalizer_spec);
    EXPECT_EQ(4, trainer.meta_pieces_.size());
    EXPECT_EQ("<unk>", trainer.meta_pieces_[0].first);
    EXPECT_EQ("<s>", trainer.meta_pieces_[1].first);
    EXPECT_EQ("</s>", trainer.meta_pieces_[2].first);
    EXPECT_EQ("<pad>", trainer.meta_pieces_[3].first);
  }

  {
    trainer_spec.set_unk_id(0);
    trainer_spec.set_bos_id(3);
    trainer_spec.set_eos_id(2);
    trainer_spec.set_pad_id(1);

    TrainerInterface trainer(trainer_spec, normalizer_spec);
    EXPECT_EQ(4, trainer.meta_pieces_.size());
    EXPECT_EQ("<unk>", trainer.meta_pieces_[0].first);
    EXPECT_EQ("<pad>", trainer.meta_pieces_[1].first);
    EXPECT_EQ("</s>", trainer.meta_pieces_[2].first);
    EXPECT_EQ("<s>", trainer.meta_pieces_[3].first);
  }

  {
    trainer_spec.set_unk_id(0);
    trainer_spec.set_bos_id(-1);
    trainer_spec.set_eos_id(1);
    trainer_spec.set_pad_id(-1);

    TrainerInterface trainer(trainer_spec, normalizer_spec);
    EXPECT_EQ(2, trainer.meta_pieces_.size());
    EXPECT_EQ("<unk>", trainer.meta_pieces_[0].first);
    EXPECT_EQ("</s>", trainer.meta_pieces_[1].first);
  }

  {
    trainer_spec.set_unk_id(0);
    trainer_spec.set_bos_id(-1);
    trainer_spec.set_eos_id(-1);
    trainer_spec.set_pad_id(-1);

    TrainerInterface trainer(trainer_spec, normalizer_spec);
    EXPECT_EQ(1, trainer.meta_pieces_.size());
    EXPECT_EQ("<unk>", trainer.meta_pieces_[0].first);
  }

  {
    trainer_spec.set_unk_id(0);
    trainer_spec.set_bos_id(1);
    trainer_spec.set_eos_id(2);
    trainer_spec.set_pad_id(-1);

    trainer_spec.add_control_symbols("<c1>");
    trainer_spec.add_control_symbols("<c2>");
    trainer_spec.add_user_defined_symbols("<u1>");
    trainer_spec.add_user_defined_symbols("<u2>");

    TrainerInterface trainer(trainer_spec, normalizer_spec);
    EXPECT_EQ(7, trainer.meta_pieces_.size());
    EXPECT_EQ("<unk>", trainer.meta_pieces_[0].first);
    EXPECT_EQ("<s>", trainer.meta_pieces_[1].first);
    EXPECT_EQ("</s>", trainer.meta_pieces_[2].first);
    EXPECT_EQ("<c1>", trainer.meta_pieces_[3].first);
    EXPECT_EQ("<c2>", trainer.meta_pieces_[4].first);
    EXPECT_EQ("<u1>", trainer.meta_pieces_[5].first);
    EXPECT_EQ("<u2>", trainer.meta_pieces_[6].first);
  }

  {
    // ID is not contiguous.
    trainer_spec.set_unk_id(0);
    trainer_spec.set_bos_id(-1);
    trainer_spec.set_eos_id(2);
    EXPECT_DEATH(TrainerInterface trainer(trainer_spec, normalizer_spec));

    // UNK is not defined.
    trainer_spec.set_unk_id(-1);
    trainer_spec.set_bos_id(0);
    trainer_spec.set_eos_id(1);
    EXPECT_DEATH(TrainerInterface trainer(trainer_spec, normalizer_spec));
  }
}

}  // namespace sentencepiece
