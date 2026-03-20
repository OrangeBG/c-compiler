#include "../include/token.h"
#include "../include/error.h"

char* get_token_name(TokenType token_type) {
  switch (token_type) {
    case TOKEN_ASTERISK:                      return "Asterisk";
    case TOKEN_ASTERISK_EQUAL:                return "Asterisk_Equal";
    case TOKEN_BITWISE_AND:                   return "Bitwise_And";
    case TOKEN_BITWISE_AND_EQUAL:             return "Bitwise_And_Equal";
    case TOKEN_BITWISE_NOT:                   return "Bitwise_Not";
    case TOKEN_BITWISE_OR:                    return "Bitwise_Or";
    case TOKEN_BITWISE_OR_EQUAL:              return "Bitwise_Or_Equal";
    case TOKEN_BITWISE_XOR:                   return "Bitwise_XOR";
    case TOKEN_BITWISE_XOR_EQUAL:             return "Bitwise_XOR_Equal";
    case TOKEN_BITWISE_LEFT_SHIFT:            return "Bitwise_Left_Shift";
    case TOKEN_BITWISE_LEFT_SHIFT_EQUAL:      return "Bitwise_Left_Shift_Equal";
    case TOKEN_BITWISE_RIGHT_SHIFT:           return "Bitwise_Right_Shift";
    case TOKEN_BITWISE_RIGHT_SHIFT_EQUAL:     return "Bitwise_Right_Shift_Equal";
    case TOKEN_BREAK:                         return "Break";
    case TOKEN_CHAR:                          return "Char";
    case TOKEN_CHARACTER_CONSTANT:            return "Character Constant";
    case TOKEN_CLOSE_BRACE:                   return "Close_Brace";
    case TOKEN_CLOSE_BRACKET:                 return "Close_Bracket";
    case TOKEN_CLOSE_PAREN:                   return "Close Paren";
    case TOKEN_COLON:                         return "Colon";
    case TOKEN_COMMA:                         return "Comma";
    case TOKEN_CONSTANT_FLOAT:                return "Float";
    case TOKEN_CONSTANT_INT:                  return "Int";
    case TOKEN_CONSTANT_LONG:                 return "Long";
    case TOKEN_CONSTANT_UNSIGNED_INT:         return "UInt";
    case TOKEN_CONSTANT_UNSIGNED_LONG:        return "ULong";
    case TOKEN_CONTINUE:                      return "Continue";
    case TOKEN_DECREMENT:                     return "Decrement";
    case TOKEN_DO:                            return "Do";
    case TOKEN_DOUBLE:                        return "Double";
    case TOKEN_ELSE:                          return "Else";
    case TOKEN_EQUAL:                         return "Equal";
    case TOKEN_EXTERN:                        return "Extern";
    case TOKEN_FOR:                           return "For";
    case TOKEN_FORWARD_SLASH:                 return "Forward_Slash";
    case TOKEN_FORWARD_SLASH_EQUAL:           return "Forward_Slash_Equal";
    case TOKEN_GOTO:                          return "Goto";
    case TOKEN_IDENTIFIER:                    return "Identifier";
    case TOKEN_IF:                            return "If";
    case TOKEN_INCREMENT:                     return "Increment";
    case TOKEN_INT:                           return "Int";
    case TOKEN_LOGICAL_AND:                   return "Logical_And"; 
    case TOKEN_LOGICAL_OR:                    return "Logical_Or";
    case TOKEN_LOGICAL_NOT:                   return "Logical_Not";
    case TOKEN_LONG:                          return "Long";
    case TOKEN_NEGATION:                      return "Negation";
    case TOKEN_NEGATION_EQUAL:                return "Negation_Equal";
    case TOKEN_OPEN_PAREN:                    return "Open_Paren";
    case TOKEN_OPEN_BRACE:                    return "Open_Brace";
    case TOKEN_OPEN_BRACKET:                  return "Open_Bracket";
    case TOKEN_PERCENT:                       return "Percent";
    case TOKEN_PERCENT_EQUAL:                 return "Percent_Equal";
    case TOKEN_PLUS:                          return "Plus";
    case TOKEN_PLUS_EQUAL:                    return "Plus_Equal";
    case TOKEN_QUESTION_MARK:                 return "Question_Mark";
    case TOKEN_RELATIONAL_EQUAL:              return "Relational_Equal";
    case TOKEN_RELATIONAL_NOT_EQUAL:          return "Relational_Not_Equal";
    case TOKEN_RELATIONAL_LESS_THAN:          return "Relational_Less_Than";
    case TOKEN_RELATIONAL_LESS_OR_EQUAL:      return "Relational_Less_Or_Equal";
    case TOKEN_RELATIONAL_GREATER_THAN:       return "Relational_Greater_Than";
    case TOKEN_RELATIONAL_GREATER_OR_EQUAL:   return "Relational_Greater_Or_Equal";
    case TOKEN_RETURN:                        return "Return";
    case TOKEN_SEMICOLON:                     return "Semicolon";
    case TOKEN_SIGNED:                        return "Signed";
    case TOKEN_STATIC:                        return "Static";
    case TOKEN_STRING_LITERAL:                return "String Literal";
    case TOKEN_UNSIGNED:                      return "Unsigned";
    case TOKEN_VOID:                          return "Void";
    case TOKEN_WHILE:                         return "While";
    case TOKEN_EOF:                           return "End_Of_File";
    default:                                  panic("Could not print lexer token");
  }
}
